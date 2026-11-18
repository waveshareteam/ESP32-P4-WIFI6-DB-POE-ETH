/* SPDX-License-Identifier: Apache-2.0 */

#include <string.h>
#include "board_ethernet.h"
#include "esp_board_manager_includes.h"
#include "esp_check.h"
#include "esp_eth.h"
#include "esp_eth_netif_glue.h"
#include "esp_event.h"

static const char *TAG = "board_ethernet";
static esp_eth_handle_t ethernet_handle;
static esp_netif_t *ethernet_netif;
static esp_eth_netif_glue_handle_t ethernet_glue;
static bool ethernet_started;

esp_err_t board_ethernet_init(void)
{
    if (ethernet_handle) {
        return ESP_OK;
    }
    esp_err_t ret = esp_netif_init();
    ESP_RETURN_ON_FALSE(ret == ESP_OK || ret == ESP_ERR_INVALID_STATE, ret, TAG, "Initialize netif failed");
    ret = esp_event_loop_create_default();
    ESP_RETURN_ON_FALSE(ret == ESP_OK || ret == ESP_ERR_INVALID_STATE, ret, TAG, "Create event loop failed");
    ESP_RETURN_ON_ERROR(esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_ETHERNET), TAG,
                        "Initialize Ethernet device failed");

    dev_ethernet_handles_t *handles = NULL;
    ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_ETHERNET, (void **)&handles);
    if (ret != ESP_OK || !handles || !handles->eth_handle) {
        ret = ret != ESP_OK ? ret : ESP_ERR_INVALID_STATE;
        goto fail_device;
    }
    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_ETH();
    ethernet_netif = esp_netif_new(&netif_config);
    if (!ethernet_netif) {
        ret = ESP_ERR_NO_MEM;
        goto fail_device;
    }
    ethernet_glue = esp_eth_new_netif_glue(handles->eth_handle);
    if (!ethernet_glue) {
        ret = ESP_ERR_NO_MEM;
        goto fail_netif;
    }
    ret = esp_netif_attach(ethernet_netif, ethernet_glue);
    if (ret == ESP_OK) {
        ret = esp_eth_start(handles->eth_handle);
    }
    if (ret == ESP_OK) {
        ethernet_handle = handles->eth_handle;
        ethernet_started = true;
        return ESP_OK;
    }
    esp_eth_del_netif_glue(ethernet_glue);
    ethernet_glue = NULL;
fail_netif:
    esp_netif_destroy(ethernet_netif);
    ethernet_netif = NULL;
fail_device:
    if (esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_ETHERNET) != ESP_OK) {
        ESP_LOGE(TAG, "Clean up Ethernet device failed");
    }
    return ret;
}

esp_err_t board_ethernet_deinit(void)
{
    if (!ethernet_handle) {
        return ESP_OK;
    }
    if (ethernet_started) {
        ESP_RETURN_ON_ERROR(esp_eth_stop(ethernet_handle), TAG, "Stop Ethernet failed");
        ethernet_started = false;
    }
    if (ethernet_glue) {
        ESP_RETURN_ON_ERROR(esp_eth_del_netif_glue(ethernet_glue), TAG, "Delete Ethernet glue failed");
        ethernet_glue = NULL;
    }
    if (ethernet_netif) {
        esp_netif_destroy(ethernet_netif);
        ethernet_netif = NULL;
    }
    ESP_RETURN_ON_ERROR(esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_ETHERNET), TAG,
                        "Deinitialize Ethernet device failed");
    ethernet_handle = NULL;
    return ESP_OK;
}

esp_err_t board_ethernet_get_info(board_ethernet_info_t *info)
{
    ESP_RETURN_ON_FALSE(info, ESP_ERR_INVALID_ARG, TAG, "Invalid Ethernet info");
    memset(info, 0, sizeof(*info));
    ESP_RETURN_ON_FALSE(ethernet_handle && ethernet_netif, ESP_ERR_INVALID_STATE, TAG, "Ethernet is not initialized");
    eth_speed_t speed = ETH_SPEED_10M;
    eth_duplex_t duplex = ETH_DUPLEX_HALF;
    ESP_RETURN_ON_ERROR(esp_netif_get_ip_info(ethernet_netif, &info->ip), TAG, "Get IP failed");
    ESP_RETURN_ON_ERROR(esp_eth_ioctl(ethernet_handle, ETH_CMD_G_MAC_ADDR, info->mac), TAG, "Get MAC failed");
    ESP_RETURN_ON_ERROR(esp_eth_ioctl(ethernet_handle, ETH_CMD_G_PHY_ADDR, &info->phy_addr), TAG, "Get PHY address failed");
    if (esp_netif_is_netif_up(ethernet_netif)) {
        ESP_RETURN_ON_ERROR(esp_eth_ioctl(ethernet_handle, ETH_CMD_G_SPEED, &speed), TAG, "Get speed failed");
        ESP_RETURN_ON_ERROR(esp_eth_ioctl(ethernet_handle, ETH_CMD_G_DUPLEX_MODE, &duplex), TAG, "Get duplex failed");
        info->link_up = true;
    }
    info->speed_mbps = speed == ETH_SPEED_100M ? 100 : 10;
    info->full_duplex = duplex == ETH_DUPLEX_FULL;
    return ESP_OK;
}
