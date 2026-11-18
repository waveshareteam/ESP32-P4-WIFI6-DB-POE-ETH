/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_netif.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    esp_netif_ip_info_t ip;
    uint8_t mac[6];
    uint32_t phy_addr;
    unsigned int speed_mbps;
    bool full_duplex;
    bool link_up;
} board_ethernet_info_t;

esp_err_t board_ethernet_init(void);
esp_err_t board_ethernet_deinit(void);
esp_err_t board_ethernet_get_info(board_ethernet_info_t *info);

#ifdef __cplusplus
}
#endif
