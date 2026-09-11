/* SPDX-License-Identifier: Apache-2.0 */

#include "sdkconfig.h"
#include "esp_check.h"
#include "bsp/ethernet.h"

static const char *TAG = "BSP-ETH";

esp_err_t bsp_eth_init(esp_eth_handle_t *eth_handle)
{
    ESP_RETURN_ON_FALSE(eth_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "Ethernet handle is NULL");
    *eth_handle = NULL;
#if CONFIG_ETH_USE_ESP32_EMAC
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = BSP_ETH_PHY_ADDR;
    phy_config.reset_gpio_num = BSP_ETH_PHY_RST;

    eth_esp32_emac_config_t emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    emac_config.smi_gpio.mdc_num = BSP_ETH_MDC;
    emac_config.smi_gpio.mdio_num = BSP_ETH_MDIO;
    emac_config.interface = EMAC_DATA_INTERFACE_RMII;
    emac_config.clock_config.rmii.clock_mode = EMAC_CLK_EXT_IN;
    emac_config.clock_config.rmii.clock_gpio = BSP_ETH_RMII_CLK;
    emac_config.emac_dataif_gpio.rmii.tx_en_num = BSP_ETH_RMII_TX_EN;
    emac_config.emac_dataif_gpio.rmii.txd0_num = BSP_ETH_RMII_TXD0;
    emac_config.emac_dataif_gpio.rmii.txd1_num = BSP_ETH_RMII_TXD1;
    emac_config.emac_dataif_gpio.rmii.crs_dv_num = BSP_ETH_RMII_CRS_DV;
    emac_config.emac_dataif_gpio.rmii.rxd0_num = BSP_ETH_RMII_RXD0;
    emac_config.emac_dataif_gpio.rmii.rxd1_num = BSP_ETH_RMII_RXD1;

    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&emac_config, &mac_config);
    ESP_RETURN_ON_FALSE(mac != NULL, ESP_ERR_NO_MEM, TAG, "Create Ethernet MAC failed");
    esp_eth_phy_t *phy = esp_eth_phy_new_generic(&phy_config);
    if (phy == NULL) {
        mac->del(mac);
        return ESP_ERR_NO_MEM;
    }
    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_err_t ret = esp_eth_driver_install(&eth_config, eth_handle);
    if (ret != ESP_OK) {
        mac->del(mac);
        phy->del(phy);
        *eth_handle = NULL;
    }
    return ret;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t bsp_eth_deinit(esp_eth_handle_t eth_handle)
{
    ESP_RETURN_ON_FALSE(eth_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "Ethernet handle is NULL");
#if CONFIG_ETH_USE_ESP32_EMAC
    esp_eth_mac_t *mac = NULL;
    esp_eth_phy_t *phy = NULL;
    ESP_RETURN_ON_ERROR(esp_eth_get_mac_instance(eth_handle, &mac), TAG, "Get Ethernet MAC failed");
    ESP_RETURN_ON_ERROR(esp_eth_get_phy_instance(eth_handle, &phy), TAG, "Get Ethernet PHY failed");
    ESP_RETURN_ON_ERROR(esp_eth_driver_uninstall(eth_handle), TAG, "Uninstall Ethernet driver failed");
    esp_err_t mac_ret = mac->del(mac);
    esp_err_t phy_ret = phy->del(phy);
    return mac_ret != ESP_OK ? mac_ret : phy_ret;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}
