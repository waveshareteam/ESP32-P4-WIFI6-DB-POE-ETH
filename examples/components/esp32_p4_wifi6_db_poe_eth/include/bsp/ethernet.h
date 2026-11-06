/*
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "driver/gpio.h"
#include "esp_eth.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_ETH_MDC           (GPIO_NUM_31)
#define BSP_ETH_MDIO          (GPIO_NUM_52)
#define BSP_ETH_PHY_RST       (GPIO_NUM_51)
#define BSP_ETH_PHY_ADDR      (1)
#define BSP_ETH_RMII_CLK      (GPIO_NUM_50)
#define BSP_ETH_RMII_TX_EN    (GPIO_NUM_49)
#define BSP_ETH_RMII_TXD0     (GPIO_NUM_34)
#define BSP_ETH_RMII_TXD1     (GPIO_NUM_35)
#define BSP_ETH_RMII_CRS_DV   (GPIO_NUM_28)
#define BSP_ETH_RMII_RXD0     (GPIO_NUM_29)
#define BSP_ETH_RMII_RXD1     (GPIO_NUM_30)

/**
 * @brief Install the board RMII MAC and generic IEEE 802.3 PHY driver.
 * @note Call once per driver lifetime. The caller owns the returned handle.
 *       Initialize the event loop, attach a netif/glue if needed, and call
 *       esp_eth_start() in the application. No network policy is installed here.
 * @param[out] eth_handle Installed driver, or NULL on failure.
 * @return ESP_OK, ESP_ERR_INVALID_ARG, ESP_ERR_NO_MEM, ESP_ERR_NOT_SUPPORTED
 *         when CONFIG_ETH_USE_ESP32_EMAC is disabled, or a driver error.
 */
esp_err_t bsp_eth_init(esp_eth_handle_t *eth_handle);

/**
 * @brief Uninstall a driver returned by bsp_eth_init() and delete its MAC/PHY.
 * @note Stop the driver and release netif glue/references before calling.
 *       If driver uninstall fails, the driver and MAC/PHY remain owned by the caller.
 *       Serialize initialization and deinitialization in the application.
 */
esp_err_t bsp_eth_deinit(esp_eth_handle_t eth_handle);

#ifdef __cplusplus
}
#endif
