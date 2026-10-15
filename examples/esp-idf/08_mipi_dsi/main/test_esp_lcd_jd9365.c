/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "bsp/display.h"

#if CONFIG_ESP_LCD_TOUCH_DRAW_TEST
#include "bsp/touch.h"
#endif

#if CONFIG_ESP_DEEP_SLEEP_TEST
#include "driver/gpio.h"
#include "esp_sleep.h"
#define WAKEUP_GPIO GPIO_NUM_5
#endif

static const char *TAG = "jd9365_test";

#if CONFIG_ESP_LCD_WHITE_SCREEN_TEST || CONFIG_ESP_LCD_TOUCH_DRAW_TEST
static esp_err_t show_solid_screen(esp_lcd_panel_handle_t panel, uint8_t pixel_value)
{
    void *frame_buffer = NULL;
    const size_t frame_buffer_size =
        BSP_LCD_H_RES * BSP_LCD_V_RES * BSP_LCD_BITS_PER_PIXEL / 8;

    ESP_RETURN_ON_ERROR(esp_lcd_dpi_panel_get_frame_buffer(panel, 1, &frame_buffer),
                        TAG, "Failed to get LCD frame buffer");
    memset(frame_buffer, pixel_value, frame_buffer_size);
    ESP_RETURN_ON_ERROR(esp_lcd_panel_draw_bitmap(
                            panel, 0, 0, BSP_LCD_H_RES, BSP_LCD_V_RES, frame_buffer),
                        TAG, "Failed to draw solid screen");
    return esp_lcd_dpi_panel_set_pattern(panel, MIPI_DSI_PATTERN_NONE);
}
#endif

static esp_err_t prepare_display(esp_lcd_panel_handle_t panel)
{
#if CONFIG_ESP_LCD_WHITE_SCREEN_TEST
    ESP_LOGI(TAG, "Show full white screen");
    ESP_RETURN_ON_ERROR(show_solid_screen(panel, 0xFF), TAG, "Failed to show white screen");
#elif CONFIG_ESP_LCD_TOUCH_DRAW_TEST
    ESP_LOGI(TAG, "Prepare black screen for touchscreen dot drawing test");
    ESP_RETURN_ON_ERROR(show_solid_screen(panel, 0x00), TAG, "Failed to show black screen");
#else
    ESP_LOGI(TAG, "Show color bar pattern drawn by hardware");
    ESP_RETURN_ON_ERROR(esp_lcd_dpi_panel_set_pattern(panel, MIPI_DSI_PATTERN_BAR_VERTICAL),
                        TAG, "Failed to show color bar pattern");
#endif
    return bsp_display_backlight_on();
}

#if CONFIG_ESP_LCD_TOUCH_DRAW_TEST
#define TOUCH_DOT_SIZE 9
#define TOUCH_POLL_INTERVAL_MS 20

#if CONFIG_BSP_LCD_COLOR_FORMAT_RGB888
static const uint8_t touch_dot_color[] = {0xFF, 0x00, 0xFF};
#else
static const uint16_t touch_dot_color = 0xF81F;
#endif

static esp_err_t draw_touch_dot(esp_lcd_panel_handle_t panel, int x, int y, const void *pixels)
{
    const int radius = TOUCH_DOT_SIZE / 2;
    x = MIN(x, BSP_LCD_H_RES - 1);
    y = MIN(y, BSP_LCD_V_RES - 1);

    const int x0 = MAX(x - radius, 0);
    const int y0 = MAX(y - radius, 0);
    const int x1 = MIN(x + radius + 1, BSP_LCD_H_RES);
    const int y1 = MIN(y + radius + 1, BSP_LCD_V_RES);

    // All pixels have the same color, so clipped rectangles use the same buffer.
    return esp_lcd_panel_draw_bitmap(panel, x0, y0, x1, y1, pixels);
}

static esp_err_t draw_touch_points(esp_lcd_panel_handle_t panel,
                                  esp_lcd_touch_handle_t touch, const void *pixels)
{
    uint8_t point_count = 0;
    esp_lcd_touch_point_data_t points[CONFIG_ESP_LCD_TOUCH_MAX_POINTS] = {0};

    ESP_RETURN_ON_ERROR(esp_lcd_touch_read_data(touch), TAG, "Failed to read touch data");
    ESP_RETURN_ON_ERROR(esp_lcd_touch_get_data(
                            touch, points, &point_count, CONFIG_ESP_LCD_TOUCH_MAX_POINTS),
                        TAG, "Failed to get touch coordinates");

    for (uint8_t i = 0; i < point_count; i++) {
        ESP_RETURN_ON_ERROR(draw_touch_dot(panel, points[i].x, points[i].y, pixels),
                            TAG, "Failed to draw touch dot");
    }
    return ESP_OK;
}

static esp_err_t run_touch_draw_test(esp_lcd_panel_handle_t panel)
{
    esp_lcd_touch_handle_t touch = NULL;
    ESP_RETURN_ON_ERROR(bsp_touch_new(NULL, &touch), TAG, "Failed to initialize touchscreen");

    uint8_t pixels[TOUCH_DOT_SIZE * TOUCH_DOT_SIZE * sizeof(touch_dot_color)];
    for (size_t i = 0; i < TOUCH_DOT_SIZE * TOUCH_DOT_SIZE; i++) {
        memcpy(pixels + i * sizeof(touch_dot_color), &touch_dot_color, sizeof(touch_dot_color));
    }

    ESP_LOGI(TAG, "Touch the screen to draw magenta dots");
    esp_err_t ret;
    while ((ret = draw_touch_points(panel, touch, pixels)) == ESP_OK) {
        // Reading the GT911 more frequently may result in false reports.
        vTaskDelay(pdMS_TO_TICKS(TOUCH_POLL_INTERVAL_MS));
    }

    bsp_touch_delete();
    return ret;
}
#endif

#if CONFIG_ESP_DEEP_SLEEP_TEST
static void run_deep_sleep_test(uint32_t wakeup_causes)
{
    if (wakeup_causes & (1UL << ESP_SLEEP_WAKEUP_GPIO)) {
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    vTaskDelay(pdMS_TO_TICKS(2000));

    const gpio_config_t wakeup_gpio_config = {
        .pin_bit_mask = 1ULL << WAKEUP_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&wakeup_gpio_config));
    ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup_on_hp_periph_powerdown(
                        1ULL << WAKEUP_GPIO, ESP_GPIO_WAKEUP_GPIO_HIGH));
    ESP_LOGI("sleep", "Entering deep sleep, GPIO%d level: %d", WAKEUP_GPIO,
             gpio_get_level(WAKEUP_GPIO));

    if (gpio_get_level(WAKEUP_GPIO) == 1) {
        ESP_LOGI("sleep", "Waiting for GPIO%d to return low", WAKEUP_GPIO);
        while (gpio_get_level(WAKEUP_GPIO) == 1) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    esp_deep_sleep_start();
}
#endif

void app_main(void)
{
#if CONFIG_ESP_DEEP_SLEEP_TEST
    const uint32_t wakeup_causes = esp_sleep_get_wakeup_causes();
    if (wakeup_causes & (1UL << ESP_SLEEP_WAKEUP_GPIO)) {
        ESP_LOGI("sleep", "Woken up by GPIO%d", WAKEUP_GPIO);
    }
#endif

    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_panel_io_handle_t lcd_io = NULL;
    ESP_LOGI(TAG, "Initialize LCD device");
    esp_err_t ret = bsp_display_new(NULL, &panel, &lcd_io);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display: %s", esp_err_to_name(ret));
        return;
    }

    ret = prepare_display(panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to prepare display: %s", esp_err_to_name(ret));
        return;
    }

#if CONFIG_ESP_LCD_TOUCH_DRAW_TEST
    (void)run_touch_draw_test(panel);
#elif CONFIG_ESP_DEEP_SLEEP_TEST
    run_deep_sleep_test(wakeup_causes);
#elif !CONFIG_ESP_LCD_WHITE_SCREEN_TEST
    vTaskDelay(pdMS_TO_TICKS(2000));
#endif
}
