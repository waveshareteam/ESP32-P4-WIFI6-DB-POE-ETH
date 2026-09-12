/*
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ESP32-P4-WIFI6-DB-POE-ETH RMII Ethernet MAC/PHY custom device.
 *
 * esp-board-manager has no built-in "ethernet" device type, so this board
 * declares `ethernet` as `type: custom` in board_devices.yaml and registers
 * the init/deinit hooks below. On success the device handle returned to the
 * application IS the esp_eth_handle_t: attach a netif/glue and call
 * esp_eth_start() from application code. No network policy is installed
 * here.
 */

#include "sdkconfig.h"

#if CONFIG_ETH_USE_ESP32_EMAC

#include "esp_board_manager_includes.h"
#include "esp_eth.h"
#include "esp_eth_mac_esp.h"
#include "esp_eth_phy_ip101.h"
#include "esp_log.h"
#include "gen_board_device_custom.h"

static const char *TAG = "CUSTOM_ETHERNET";

static int custom_ethernet_init(void *cfg, int cfg_size, void **device_handle)
{
    if (cfg == NULL || device_handle == NULL || cfg_size != sizeof(dev_custom_ethernet_config_t)) {
        ESP_LOGE(TAG, "Invalid Ethernet configuration");
        return ESP_ERR_INVALID_ARG;
    }

    const dev_custom_ethernet_config_t *config = (const dev_custom_ethernet_config_t *)cfg;

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = config->phy_addr;
    phy_config.reset_gpio_num = config->phy_rst_gpio;

    eth_esp32_emac_config_t emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    emac_config.smi_gpio.mdc_num = config->mdc_gpio;
    emac_config.smi_gpio.mdio_num = config->mdio_gpio;
    emac_config.interface = EMAC_DATA_INTERFACE_RMII;
    /* 50 MHz RMII reference clock is supplied by the PHY, not driven out by the chip. */
    emac_config.clock_config.rmii.clock_mode = EMAC_CLK_EXT_IN;
    emac_config.clock_config.rmii.clock_gpio = config->rmii_clk_gpio;
    emac_config.emac_dataif_gpio.rmii.tx_en_num = config->rmii_tx_en_gpio;
    emac_config.emac_dataif_gpio.rmii.txd0_num = config->rmii_txd0_gpio;
    emac_config.emac_dataif_gpio.rmii.txd1_num = config->rmii_txd1_gpio;
    emac_config.emac_dataif_gpio.rmii.crs_dv_num = config->rmii_crs_dv_gpio;
    emac_config.emac_dataif_gpio.rmii.rxd0_num = config->rmii_rxd0_gpio;
    emac_config.emac_dataif_gpio.rmii.rxd1_num = config->rmii_rxd1_gpio;

    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&emac_config, &mac_config);
    if (mac == NULL) {
        ESP_LOGE(TAG, "Failed to create Ethernet MAC");
        return ESP_ERR_NO_MEM;
    }

    esp_eth_phy_t *phy = esp_eth_phy_new_ip101(&phy_config);
    if (phy == NULL) {
        ESP_LOGE(TAG, "Failed to create Ethernet PHY");
        mac->del(mac);
        return ESP_ERR_NO_MEM;
    }

    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    esp_err_t ret = esp_eth_driver_install(&eth_config, &eth_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install Ethernet driver: %s", esp_err_to_name(ret));
        mac->del(mac);
        phy->del(phy);
        return ret;
    }

    *device_handle = (void *)eth_handle;
    ESP_LOGI(TAG, "Ethernet initialized: PHY addr=%d, MDC=%d, MDIO=%d, PHY RST=%d",
             (int)config->phy_addr, (int)config->mdc_gpio, (int)config->mdio_gpio,
             (int)config->phy_rst_gpio);
    return ESP_OK;
}

static int custom_ethernet_deinit(void *device_handle)
{
    if (device_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_eth_handle_t eth_handle = (esp_eth_handle_t)device_handle;
    esp_eth_mac_t *mac = NULL;
    esp_eth_phy_t *phy = NULL;
    esp_err_t ret = esp_eth_get_mac_instance(eth_handle, &mac);
    esp_err_t phy_ret = esp_eth_get_phy_instance(eth_handle, &phy);
    esp_err_t uninstall_ret = esp_eth_driver_uninstall(eth_handle);

    esp_err_t mac_del_ret = (ret == ESP_OK && mac != NULL) ? mac->del(mac) : ret;
    esp_err_t phy_del_ret = (phy_ret == ESP_OK && phy != NULL) ? phy->del(phy) : phy_ret;

    if (uninstall_ret != ESP_OK) {
        return uninstall_ret;
    }
    if (mac_del_ret != ESP_OK) {
        return mac_del_ret;
    }
    return phy_del_ret;
}

CUSTOM_DEVICE_IMPLEMENT(ethernet, custom_ethernet_init, custom_ethernet_deinit);

#endif /* CONFIG_ETH_USE_ESP32_EMAC */
