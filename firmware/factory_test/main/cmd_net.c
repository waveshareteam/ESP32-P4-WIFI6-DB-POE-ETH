#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_console.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "iperf_cmd.h"
#include "ping_cmd.h"
#include "cmd_net.h"

static EventGroupHandle_t s_events;
static void ip_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    ip_event_got_ip_t *ev = data;
    if (id == IP_EVENT_STA_GOT_IP) {
        printf("wifi0: ip " IPSTR " mask " IPSTR " gw " IPSTR "\n",
               IP2STR(&ev->ip_info.ip), IP2STR(&ev->ip_info.netmask), IP2STR(&ev->ip_info.gw));
        xEventGroupSetBits(s_events, BIT_WIFI_IP);
    }
}

static void events_init_once(void)
{
    if (!s_events) {
        s_events = xEventGroupCreate();
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, ip_event_handler, NULL));
    }
}

EventGroupHandle_t net_events(void)
{
    events_init_once();
    return s_events;
}

static int cmd_ifup(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: ifup wifi0\n");
        return 1;
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
        printf("usage: ifdown wifi0\n");
        return 1;
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
        printf("no interfaces up (use: ifup wifi0)\n");
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
        netif = esp_netif_next_unsafe(netif);
    }
    wifi_print_status();
    return 0;
}

void register_net_commands(void)
{
    const esp_console_cmd_t cmds[] = {
        { .command = "ifup",     .help = "Bring an interface up. Usage: ifup wifi0", .func = cmd_ifup },
        { .command = "ifdown",   .help = "Bring an interface down. Usage: ifdown wifi0", .func = cmd_ifdown },
        { .command = "ifconfig", .help = "Show interfaces (MAC, IP, RSSI)", .func = cmd_ifconfig },
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
