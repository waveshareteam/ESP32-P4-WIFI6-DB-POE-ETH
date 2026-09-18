#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_hosted.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "cmd_net.h"
#include "report.h"

static const char *TAG = "wifi";
static char s_ssid[33];
static char s_pass[65];
static bool s_hosted_ready;
static bool s_wifi_started;
static esp_netif_t *s_netif;

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (id == WIFI_EVENT_STA_CONNECTED) {
        printf("wifi0: associated\n");
    } else if (id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *ev = data;
        printf("wifi0: disconnected, reason %d\n", ev->reason);
    }
}

/* The ESP-Hosted transport is not auto-initialised before app_main
 * (CONFIG_ESP_HOSTED_AUTO_CALL_INIT_BEFORE_APP_MAIN=n) so a board whose C5 is
 * not flashed still boots into the console. */
static esp_err_t hosted_ready(void)
{
    if (s_hosted_ready) {
        return ESP_OK;
    }
    printf("wifi0: starting ESP-Hosted link to ESP32-C5 (SDIO slot 1)...\n");
    if (esp_hosted_init() != 0) {
        printf("esp_hosted_init failed (is the C5 flashed with examples/esp-idf/15_wifi_iperf/cp?)\n");
        return ESP_FAIL;
    }
    if (esp_hosted_connect_to_slave() != 0) {
        printf("esp_hosted_connect_to_slave failed\n");
        return ESP_FAIL;
    }
    esp_hosted_coprocessor_fwver_t ver = {0};
    if (esp_hosted_get_coprocessor_fwversion(&ver) == 0) {
        printf("wifi0: coprocessor fw %u.%u.%u\n",
               (unsigned)ver.major1, (unsigned)ver.minor1, (unsigned)ver.patch1);
    }
    s_hosted_ready = true;
    return ESP_OK;
}

esp_err_t wifi_up(void)
{
    if (s_wifi_started) {
        printf("wifi0 already up\n");
        return ESP_OK;
    }
    if (!s_ssid[0]) {
        printf("set credentials first: iwconfig wifi0 essid <ssid> key <pwd>\n");
        return ESP_ERR_INVALID_STATE;
    }
    ESP_RETURN_ON_ERROR(hosted_ready(), TAG, "hosted");
    EventGroupHandle_t ev = net_events();
    xEventGroupClearBits(ev, BIT_WIFI_IP);
    if (!s_netif) {
        s_netif = esp_netif_create_default_wifi_sta();
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    }
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "init");
    wifi_config_t wc = {0};
    strlcpy((char *)wc.sta.ssid, s_ssid, sizeof(wc.sta.ssid));
    strlcpy((char *)wc.sta.password, s_pass, sizeof(wc.sta.password));
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wc), TAG, "config");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "start");
    esp_wifi_set_ps(WIFI_PS_NONE);
    s_wifi_started = true;
    printf("wifi0: connecting to '%s' (20 s)...\n", s_ssid);
    EventBits_t bits = xEventGroupWaitBits(ev, BIT_WIFI_IP, pdFALSE, pdFALSE, pdMS_TO_TICKS(20000));
    if (bits & BIT_WIFI_IP) {
        esp_netif_set_default_netif(s_netif);
        wifi_ap_record_t ap;
        char value[24] = "";
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
            snprintf(value, sizeof(value), "rssi %d ch %d", ap.rssi, ap.primary);
            printf("wifi0: %s\n", value);
        }
        report_set("wifi.link", REPORT_PASS, value, s_ssid);
        return ESP_OK;
    }
    printf("wifi0: no IP address\n");
    report_set("wifi.link", REPORT_FAIL, NULL, "no ip");
    return ESP_ERR_TIMEOUT;
}

esp_err_t wifi_down(void)
{
    if (!s_wifi_started) {
        return ESP_OK;
    }
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
    s_wifi_started = false;
    return ESP_OK;
}

void wifi_print_status(void)
{
    if (!s_wifi_started) {
        return;
    }
    wifi_ap_record_t ap;
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        printf("wifi0: ESSID \"%s\"  channel %d  RSSI %d dBm\n", (const char *)ap.ssid, ap.primary, ap.rssi);
    } else {
        printf("wifi0: not associated\n");
    }
}

/* iwconfig wifi0 essid <ssid> [key <pwd>]   |   iwconfig */
int cmd_iwconfig(int argc, char **argv)
{
    if (argc == 1) {
        wifi_print_status();
        printf("configured essid: \"%s\"\n", s_ssid);
        return 0;
    }
    if (argc >= 4 && strcmp(argv[1], "wifi0") == 0 && strcmp(argv[2], "essid") == 0) {
        strlcpy(s_ssid, argv[3], sizeof(s_ssid));
        s_pass[0] = '\0';
        if (argc >= 6 && strcmp(argv[4], "key") == 0) {
            strlcpy(s_pass, argv[5], sizeof(s_pass));
        }
        printf("wifi0: essid \"%s\" set; run: ifup wifi0\n", s_ssid);
        return 0;
    }
    printf("usage: iwconfig wifi0 essid <ssid> key <pwd>\n");
    return 1;
}
