/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_log.h"
#include "esp_board_device.h"
#include "dev_display_lcd.h"
#include "lcd_brightness.h"

static const char *TAG = "jd9365_test";
static void *s_brightness;

static esp_err_t prepare_display(esp_lcd_panel_handle_t panel)
{
    ESP_LOGI(TAG, "Show color bar pattern drawn by hardware");
    ESP_RETURN_ON_ERROR(esp_lcd_dpi_panel_set_pattern(panel, MIPI_DSI_PATTERN_BAR_VERTICAL),
                        TAG, "Failed to show color bar pattern");
    return esp_custom_lcd_brightness_set(s_brightness, 100);
}

void app_main(void)
{
    dev_display_lcd_config_t *display_config = NULL;
    dev_display_lcd_handles_t *display_handles = NULL;
    ESP_ERROR_CHECK(esp_board_device_get_config("display_lcd", (void **)&display_config));
    if (display_config == NULL || display_config->bits_per_pixel != 16) {
        ESP_LOGE(TAG, "This test requires an RGB565 display configuration");
        return;
    }
    ESP_ERROR_CHECK(esp_board_device_init("display_lcd"));
    ESP_ERROR_CHECK(esp_board_device_get_handle("display_lcd", (void **)&display_handles));
    ESP_ERROR_CHECK(esp_board_device_init("lcd_brightness"));
    ESP_ERROR_CHECK(esp_board_device_get_handle("lcd_brightness", &s_brightness));
    esp_lcd_panel_handle_t panel = display_handles->panel_handle;
    esp_err_t ret;

    ret = prepare_display(panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to prepare display: %s", esp_err_to_name(ret));
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
}
