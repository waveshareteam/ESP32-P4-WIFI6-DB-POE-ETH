/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_board_device.h"
#include "dev_display_lcd.h"
#include "dev_lcd_touch.h"
#include "lcd_brightness.h"
#include "esp_lv_adapter.h"

#include "gpio_monitor.h"

#define GPIO_MONITOR_NAME_LEN 8

#ifdef CONFIG_GPIO_MONITOR_DEFAULT_MODE_OUTPUT
#define GPIO_MONITOR_INITIAL_MODE GPIO_MODE_OUTPUT
#else
#define GPIO_MONITOR_INITIAL_MODE GPIO_MODE_INPUT
#endif

#if defined(CONFIG_GPIO_MONITOR_PULL_UP)
#define GPIO_MONITOR_PULL_UP   GPIO_PULLUP_ENABLE
#define GPIO_MONITOR_PULL_DOWN GPIO_PULLDOWN_DISABLE
#elif defined(CONFIG_GPIO_MONITOR_PULL_UP_DOWN)
#define GPIO_MONITOR_PULL_UP   GPIO_PULLUP_ENABLE
#define GPIO_MONITOR_PULL_DOWN GPIO_PULLDOWN_ENABLE
#elif defined(CONFIG_GPIO_MONITOR_FLOATING)
#define GPIO_MONITOR_PULL_UP   GPIO_PULLUP_DISABLE
#define GPIO_MONITOR_PULL_DOWN GPIO_PULLDOWN_DISABLE
#else /* CONFIG_GPIO_MONITOR_PULL_DOWN (default) */
#define GPIO_MONITOR_PULL_UP   GPIO_PULLUP_DISABLE
#define GPIO_MONITOR_PULL_DOWN GPIO_PULLDOWN_ENABLE
#endif

static const gpio_num_t s_header_gpios[] = {
    GPIO_NUM_2, GPIO_NUM_3, GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_20,
    GPIO_NUM_21, GPIO_NUM_22, GPIO_NUM_23, GPIO_NUM_24, GPIO_NUM_25,
    GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_32, GPIO_NUM_33, GPIO_NUM_36,
    GPIO_NUM_45, GPIO_NUM_46, GPIO_NUM_47, GPIO_NUM_48, GPIO_NUM_53,
};

static gpio_monitor_pin_config_t s_gpio_pins[GPIO_NUM_MAX];
static char s_gpio_pin_names[GPIO_NUM_MAX][GPIO_MONITOR_NAME_LEN];

static esp_err_t gpio_monitor_init_from_board(void)
{
    size_t gpio_count = sizeof(s_header_gpios) / sizeof(s_header_gpios[0]);
    const gpio_num_t *gpios = s_header_gpios;
    if (gpios == NULL || gpio_count == 0 || gpio_count > GPIO_NUM_MAX) {
        return ESP_ERR_INVALID_SIZE;
    }

    for (size_t i = 0; i < gpio_count; i++) {
        snprintf(s_gpio_pin_names[i], sizeof(s_gpio_pin_names[i]), "GPIO%d", gpios[i]);
        s_gpio_pins[i] = (gpio_monitor_pin_config_t) {
            .name = s_gpio_pin_names[i],
            .gpio_num = gpios[i],
            .pull_up_en = GPIO_MONITOR_PULL_UP,
            .pull_down_en = GPIO_MONITOR_PULL_DOWN,
            .initial_level = 0,
        };
    }

    const gpio_monitor_config_t config = {
        .pins = s_gpio_pins,
        .pin_count = gpio_count,
        .initial_mode = GPIO_MONITOR_INITIAL_MODE,
        .refresh_period_ms = 100,
    };
    return gpio_monitor_init(&config);
}

static const char *TAG = "gpio_monitor";

void app_main(void)
{
    ESP_ERROR_CHECK(gpio_monitor_init_from_board());
    dev_display_lcd_config_t *display_config = NULL;
    dev_display_lcd_handles_t *display_handles = NULL;
    dev_lcd_touch_handles_t *touch_handles = NULL;
    void *brightness = NULL;
    lv_display_t *display = NULL;
    lv_indev_t *touch = NULL;
    bool adapter_initialized = false;
    bool display_initialized = false;
    bool touch_initialized = false;
    bool brightness_initialized = false;

    esp_err_t ret = esp_board_device_get_config("display_lcd", (void **)&display_config);
    if (ret != ESP_OK || display_config == NULL) {
        goto cleanup;
    }
    ret = esp_board_device_init("display_lcd");
    if (ret != ESP_OK) {
        goto cleanup;
    }
    display_initialized = true;
    ret = esp_board_device_get_handle("display_lcd", (void **)&display_handles);
    if (ret != ESP_OK || display_handles == NULL) {
        goto cleanup;
    }
    ret = esp_board_device_init("lcd_touch");
    if (ret != ESP_OK) {
        goto cleanup;
    }
    touch_initialized = true;
    ret = esp_board_device_get_handle("lcd_touch", (void **)&touch_handles);
    if (ret != ESP_OK || touch_handles == NULL) {
        goto cleanup;
    }
    ret = esp_board_device_init("lcd_brightness");
    if (ret != ESP_OK) {
        goto cleanup;
    }
    brightness_initialized = true;
    ret = esp_board_device_get_handle("lcd_brightness", &brightness);
    if (ret != ESP_OK || brightness == NULL) {
        goto cleanup;
    }
    esp_lv_adapter_config_t adapter_config = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    ret = esp_lv_adapter_init(&adapter_config);
    if (ret != ESP_OK) {
        goto cleanup;
    }
    adapter_initialized = true;
    esp_lv_adapter_display_config_t adapter_display = ESP_LV_ADAPTER_DISPLAY_MIPI_DEFAULT_CONFIG(
        display_handles->panel_handle, display_handles->io_handle, display_config->lcd_width,
        display_config->lcd_height, ESP_LV_ADAPTER_ROTATE_0);
    adapter_display.tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL;
    display = esp_lv_adapter_register_display(&adapter_display);
    if (display == NULL) {
        ret = ESP_FAIL;
        goto cleanup;
    }
    esp_lv_adapter_touch_config_t adapter_touch = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(display, touch_handles->touch_handle);
    touch = esp_lv_adapter_register_touch(&adapter_touch);
    if (touch == NULL) {
        ret = ESP_FAIL;
        goto cleanup;
    }
    ret = esp_lv_adapter_start();
    if (ret != ESP_OK) {
        goto cleanup;
    }
    ret = esp_custom_lcd_brightness_set(brightness, 100);
    if (ret != ESP_OK) {
        goto cleanup;
    }
    ret = esp_lv_adapter_lock(-1);
    if (ret != ESP_OK) {
        goto cleanup;
    }
    ret = gpio_monitor_start_ui();
    esp_lv_adapter_unlock();
    if (ret != ESP_OK) {
        goto cleanup;
    }
    return;

cleanup:
    ESP_LOGE(TAG, "Failed to initialize display: %s", esp_err_to_name(ret != ESP_OK ? ret : ESP_FAIL));
    if (touch != NULL) {
        esp_lv_adapter_unregister_touch(touch);
    }
    if (display != NULL) {
        esp_lv_adapter_unregister_display(display);
    }
    if (adapter_initialized) {
        esp_lv_adapter_deinit();
    }
    if (brightness_initialized) {
        esp_board_device_deinit("lcd_brightness");
    }
    if (touch_initialized) {
        esp_board_device_deinit("lcd_touch");
    }
    if (display_initialized) {
        esp_board_device_deinit("display_lcd");
    }
}
