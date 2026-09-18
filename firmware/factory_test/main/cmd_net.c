#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_board_device.h"
#include "esp_check.h"
#include "esp_console.h"
#include "esp_eth.h"
#include "esp_eth_netif_glue.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "iperf_cmd.h"
#include "ping_cmd.h"
#include "cmd_net.h"
#include "report.h"

static const char *TAG = "net";

static EventGroupHandle_t s_events;
static esp_netif_t *s_eth_netif;
static esp_eth_netif_glue_handle_t s_eth_glue;
static esp_eth_handle_t s_eth;
static int s_eth_speed_mbps;

bool net_eth_is_up(void)
{
    return s_eth != NULL;
}

static void eth_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (id == ETHERNET_EVENT_CONNECTED) {
        eth_speed_t speed;
        eth_duplex_t duplex;
        esp_eth_ioctl(s_eth, ETH_CMD_G_SPEED, &speed);
        esp_eth_ioctl(s_eth, ETH_CMD_G_DUPLEX_MODE, &duplex);
        s_eth_speed_mbps = speed == ETH_SPEED_100M ? 100 : 10;
        printf("eth0: link up, %d Mbps %s duplex\n", s_eth_speed_mbps,
               duplex == ETH_DUPLEX_FULL ? "full" : "half");
    } else if (id == ETHERNET_EVENT_DISCONNECTED) {
        printf("eth0: link down\n");
    }
}

static void ip_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    ip_event_got_ip_t *ev = data;
    if (id == IP_EVENT_ETH_GOT_IP) {
        printf("eth0: ip " IPSTR " mask " IPSTR " gw " IPSTR "\n",
               IP2STR(&ev->ip_info.ip), IP2STR(&ev->ip_info.netmask), IP2STR(&ev->ip_info.gw));
        xEventGroupSetBits(s_events, BIT_ETH_IP);
    } else if (id == IP_EVENT_STA_GOT_IP) {
        printf("wifi0: ip " IPSTR " mask " IPSTR " gw " IPSTR "\n",
               IP2STR(&ev->ip_info.ip), IP2STR(&ev->ip_info.netmask), IP2STR(&ev->ip_info.gw));
        xEventGroupSetBits(s_events, BIT_WIFI_IP);
    }
}

static void events_init_once(void)
{
    if (!s_events) {
        s_events = xEventGroupCreate();
        ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, eth_event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, ip_event_handler, NULL));
    }
}

EventGroupHandle_t net_events(void)
{
    events_init_once();
    return s_events;
}

static esp_err_t eth_up(void)
{
    if (s_eth) {
        printf("eth0 already up\n");
        return ESP_OK;
    }
    events_init_once();
    xEventGroupClearBits(s_events, BIT_ETH_IP);
    ESP_RETURN_ON_ERROR(esp_board_device_init("ethernet"), TAG, "ethernet device");
    ESP_RETURN_ON_ERROR(esp_board_device_get_handle("ethernet", (void **)&s_eth), TAG, "handle");
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    s_eth_netif = esp_netif_new(&cfg);
    s_eth_glue = esp_eth_new_netif_glue(s_eth);
    ESP_RETURN_ON_ERROR(esp_netif_attach(s_eth_netif, s_eth_glue), TAG, "attach");
    ESP_RETURN_ON_ERROR(esp_eth_start(s_eth), TAG, "start");
    printf("eth0: waiting for link + DHCP (15 s)...\n");
    EventBits_t bits = xEventGroupWaitBits(s_events, BIT_ETH_IP, pdFALSE, pdFALSE, pdMS_TO_TICKS(15000));
    if (bits & BIT_ETH_IP) {
        esp_netif_set_default_netif(s_eth_netif);
        char value[24];
        snprintf(value, sizeof(value), "%d Mbps", s_eth_speed_mbps);
        report_set("eth.link", REPORT_PASS, value, NULL);
        return ESP_OK;
    }
    printf("eth0: no IP address (cable? DHCP server?)\n");
    report_set("eth.link", REPORT_FAIL, NULL, "no ip");
    return ESP_ERR_TIMEOUT;
}

static esp_err_t eth_down(void)
{
    if (!s_eth) {
        return ESP_OK;
    }
    esp_eth_stop(s_eth);
    esp_eth_del_netif_glue(s_eth_glue);
    esp_netif_destroy(s_eth_netif);
    s_eth_glue = NULL;
    s_eth_netif = NULL;
    s_eth = NULL;
    return esp_board_device_deinit("ethernet");
}

static int cmd_ifup(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: ifup eth0|wifi0\n");
        return 1;
    }
    if (strcmp(argv[1], "eth0") == 0) {
        return eth_up() == ESP_OK ? 0 : 1;
    }
    if (strcmp(argv[1], "wifi0") == 0) {
        return wifi_up() == ESP_OK ? 0 : 1;
    }
    printf("unknown interface %s\n", argv[1]);
    return 1;
}

static int cmd_ifdown(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: ifdown eth0|wifi0\n");
        return 1;
    }
    if (strcmp(argv[1], "eth0") == 0) {
        return eth_down() == ESP_OK ? 0 : 1;
    }
    if (strcmp(argv[1], "wifi0") == 0) {
        return wifi_down() == ESP_OK ? 0 : 1;
    }
    printf("unknown interface %s\n", argv[1]);
    return 1;
}

static int cmd_ifconfig(int argc, char **argv)
{
    esp_netif_t *netif = esp_netif_next_unsafe(NULL);
    esp_netif_t *def = esp_netif_get_default_netif();
    if (!netif) {
        printf("no interfaces up (use: ifup eth0 | ifup wifi0)\n");
    }
    while (netif) {
        uint8_t mac[6] = {0};
        esp_netif_ip_info_t ip;
        esp_netif_get_mac(netif, mac);
        esp_netif_get_ip_info(netif, &ip);
        printf("%s%s\n", esp_netif_get_desc(netif), netif == def ? " (default route)" : "");
        printf("    HWaddr %02x:%02x:%02x:%02x:%02x:%02x  %s\n",
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
               esp_netif_is_netif_up(netif) ? "UP" : "DOWN");
        printf("    inet " IPSTR "  mask " IPSTR "  gw " IPSTR "\n",
               IP2STR(&ip.ip), IP2STR(&ip.netmask), IP2STR(&ip.gw));
        if (netif == s_eth_netif) {
            printf("    link %d Mbps\n", s_eth_speed_mbps);
        }
        netif = esp_netif_next_unsafe(netif);
    }
    wifi_print_status();
    return 0;
}

void register_net_commands(void)
{
    const esp_console_cmd_t cmds[] = {
        { .command = "ifup",     .help = "Bring an interface up. Usage: ifup eth0 | ifup wifi0", .func = cmd_ifup },
        { .command = "ifdown",   .help = "Bring an interface down. Usage: ifdown eth0 | ifdown wifi0", .func = cmd_ifdown },
        { .command = "ifconfig", .help = "Show interfaces (MAC, IP, link speed, RSSI)", .func = cmd_ifconfig },
        { .command = "iwconfig", .help = "Wi-Fi credentials/status. Usage: iwconfig wifi0 essid <ssid> key <pwd> | iwconfig", .func = cmd_iwconfig },
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }
    /* iperf -s | -c <ip> [-u] [-t N] [-B <local ip>] | iperf -a */
    ESP_ERROR_CHECK(iperf_cmd_register_iperf());
    /* ping <host> [-c N] */
    ping_cmd_register_ping();
}
