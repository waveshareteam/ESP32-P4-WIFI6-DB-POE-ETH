#pragma once

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BIT_ETH_IP  (1 << 0)
#define BIT_WIFI_IP (1 << 1)

/* ifup / ifdown / ifconfig / iwconfig, plus the iperf and ping commands */
void register_net_commands(void);

bool net_eth_is_up(void);
EventGroupHandle_t net_events(void);   /* IP_EVENT bits shared with cmd_wifi.c */

/* provided by cmd_wifi.c */
esp_err_t wifi_up(void);
esp_err_t wifi_down(void);
int cmd_iwconfig(int argc, char **argv);
void wifi_print_status(void);

#ifdef __cplusplus
}
#endif
