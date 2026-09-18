#include <stdio.h>
#include "esp_console.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "cmd_audio.h"
#include "cmd_camera.h"
#include "cmd_display.h"
#include "cmd_file.h"
#include "cmd_gpio.h"
#include "cmd_i2c.h"
#include "cmd_net.h"
#include "cmd_storage.h"
#include "cmd_sys.h"
#include "cmd_usb.h"
#include "cmd_test.h"
#include "jobs.h"

static const char *TAG = "main";

static void register_all_commands(void)
{
    register_sys_commands();
    register_job_commands();
    register_test_commands();
    register_i2c_commands();
    register_storage_commands();
    register_usb_commands();
    register_net_commands();
    register_display_commands();
    register_camera_commands();
    register_audio_commands();
    register_gpio_commands();
    register_file_commands();
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "waveshare>";
    repl_config.max_cmdline_length = 256;
    repl_config.task_stack_size = 16384;
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));

    register_all_commands();

    printf("\nESP32-P4-WIFI6-DB-POE-ETH factory test console\n");
    printf("Type 'help' for the command list. Nothing is initialised until you use it.\n\n");
    ESP_LOGI(TAG, "console ready");
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
