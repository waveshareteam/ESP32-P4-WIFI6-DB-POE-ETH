/*
 * SPDX-FileCopyrightText: 2018-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <string.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_event.h"
#include "esp_eth.h"
#include "esp_netif.h"
#include "ethernet_init.h"
#include "esp_vfs_fat.h"
#include "cmd_system.h"
#include "cmd_ethernet.h"

#include "iperf_cmd.h"
#include "esp_board_device.h"
#include "dev_ethernet.h"

static const char *TAG = "eth_example";

static esp_eth_handle_t s_eth_handle;

static SemaphoreHandle_t ip_got_sem;

#if CONFIG_EXAMPLE_STORE_HISTORY

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

static void initialize_filesystem(void)
{
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 4,
        .format_if_mount_failed = true
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }
}
#endif // CONFIG_EXAMPLE_STORE_HISTORY

static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    xSemaphoreGive(ip_got_sem);
}

void init_ethernet_and_netif(void)
{
    ip_got_sem = xSemaphoreCreateBinary();
    if (ip_got_sem == NULL) {
        ESP_LOGE(TAG, "Failed to create semaphore");
        return;
    }

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(esp_board_device_init("ethernet"));
    dev_ethernet_handles_t *ethernet_handles = NULL;
    ESP_ERROR_CHECK(esp_board_device_get_handle("ethernet", (void **)&ethernet_handles));
    if (ethernet_handles == NULL) {
        ESP_ERROR_CHECK(ESP_ERR_INVALID_STATE);
    }
    s_eth_handle = ethernet_handles->eth_handle;

    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&cfg);
    ESP_ERROR_CHECK(eth_netif != NULL ? ESP_OK : ESP_ERR_NO_MEM);
    esp_eth_netif_glue_handle_t eth_glue = esp_eth_new_netif_glue(s_eth_handle);
    ESP_ERROR_CHECK(eth_glue != NULL ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, eth_glue));

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    ESP_ERROR_CHECK(esp_eth_start(s_eth_handle));

    if (xSemaphoreTake(ip_got_sem, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Timeout waiting for ETH IP");
    }
}

void app_main(void)
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
#if CONFIG_EXAMPLE_STORE_HISTORY
    initialize_filesystem();
    repl_config.history_save_path = HISTORY_PATH;
#endif
    repl_config.prompt = "iperf>";
    // init console REPL environment
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));

    // init Ethernet and netif
    init_ethernet_and_netif();

    /* Register commands */
    register_system_common();
    iperf_cmd_register_iperf();
    register_ethernet_commands();

    printf("\n =======================================================\n");
    printf(" |       Steps to Test Ethernet Bandwidth              |\n");
    printf(" |                                                     |\n");
    printf(" |  1. Enter 'help', check all supported commands      |\n");
    printf(" |  2. Wait ESP32 to get IP from DHCP                  |\n");
    printf(" |  3. Enter 'ethernet info', optional                 |\n");
    printf(" |  4. Server: 'iperf -u -s -i 3'                      |\n");
    printf(" |  5. Client: 'iperf -u -c SERVER_IP -t 60 -i 3'      |\n");
    printf(" |                                                     |\n");
    printf(" =======================================================\n\n");

    // start console REPL
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
