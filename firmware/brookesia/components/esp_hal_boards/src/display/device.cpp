/*
 * SPDX-FileCopyrightText: 2026 Waveshare Electronics
 * SPDX-License-Identifier: Apache-2.0
 */
#include "esp_hal_boards/display_device.hpp"

#include <algorithm>
#include <cstring>

#include "sdkconfig.h"
#include "esp_board_manager_includes.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "lcd_brightness.h"

static const char *TAG = "EspHalBoardsDisplay";
static void *s_lcd_brightness_handle = nullptr;

namespace esp_hal_boards {

DisplayDevice &DisplayDevice::get_instance()
{
    static DisplayDevice instance;
    return instance;
}

lv_display_t *DisplayDevice::init()
{
    if (display_ != nullptr) {
        return display_;
    }

    esp_err_t ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Init display_lcd failed: %s", esp_err_to_name(ret));
        return nullptr;
    }
    display_device_initialized_ = true;

    ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_TOUCH);
    if (ret == ESP_OK) {
        touch_device_initialized_ = true;
    } else {
        ESP_LOGW(TAG, "Init lcd_touch failed: %s, continue without touch", esp_err_to_name(ret));
    }

    auto fail_init = [this](const char *message, esp_err_t error) -> lv_display_t * {
        ESP_LOGE(TAG, "%s: %s", message, esp_err_to_name(error));
        release_after_failed_init();
        return nullptr;
    };

    dev_display_lcd_handles_t *lcd_handles = nullptr;
    ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, (void **)&lcd_handles);
    if (ret != ESP_OK || lcd_handles == nullptr) {
        return fail_init("Get display_lcd handle failed", ret == ESP_OK ? ESP_ERR_INVALID_STATE : ret);
    }

    dev_display_lcd_config_t *lcd_cfg = nullptr;
    ret = esp_board_manager_get_device_config(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, (void **)&lcd_cfg);
    if (ret != ESP_OK || lcd_cfg == nullptr) {
        return fail_init("Get display_lcd config failed", ret == ESP_OK ? ESP_ERR_INVALID_STATE : ret);
    }
    if (lcd_cfg->sub_type == nullptr || std::strcmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_DSI) != 0) {
        return fail_init("Unsupported LCD sub_type", ESP_ERR_NOT_SUPPORTED);
    }
    if (lcd_cfg->frame_format != DEV_DISPLAY_LCD_FRAME_FORMAT_RGB565_LE &&
            lcd_cfg->frame_format != DEV_DISPLAY_LCD_FRAME_FORMAT_RGB565_BE) {
        return fail_init("Unsupported LCD frame format", ESP_ERR_NOT_SUPPORTED);
    }
    if (lcd_cfg->swap_xy || lcd_cfg->mirror_x || lcd_cfg->mirror_y) {
        return fail_init("LVGL adapter requires an unmirrored panel", ESP_ERR_NOT_SUPPORTED);
    }

    const uint8_t panel_fb_count = lcd_cfg->sub_cfg.dsi.dpi_config.num_fbs;
    esp_lv_adapter_tear_avoid_mode_t tear_avoid_mode;
    switch (panel_fb_count) {
    case 1:
        tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_NONE;
        break;
    case 2:
        tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_DOUBLE_DIRECT;
        break;
    case 3:
        tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL;
        break;
    default:
        return fail_init("Unsupported DSI framebuffer count", ESP_ERR_NOT_SUPPORTED);
    }

    const uint8_t required_fb_count = esp_lv_adapter_get_required_frame_buffer_count(
                                         tear_avoid_mode, ESP_LV_ADAPTER_ROTATE_0);
    if (required_fb_count != panel_fb_count) {
        ESP_LOGE(TAG, "DSI framebuffer count mismatch (panel: %u, adapter: %u)",
                 static_cast<unsigned>(panel_fb_count), static_cast<unsigned>(required_fb_count));
        release_after_failed_init();
        return nullptr;
    }

    // Keep LVGL on core 1 with a PSRAM-backed stack for the high-resolution UI.
    esp_lv_adapter_config_t adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    adapter_cfg.task_stack_size = 16 * 1024;
    adapter_cfg.task_priority = 10;
    adapter_cfg.task_core_id = 1;
    adapter_cfg.stack_in_psram = true;
    ret = esp_lv_adapter_init(&adapter_cfg);
    if (ret != ESP_OK) {
        return fail_init("Init LVGL adapter failed", ret);
    }
    adapter_initialized_ = true;

    esp_lv_adapter_display_config_t disp_cfg = ESP_LV_ADAPTER_DISPLAY_MIPI_DEFAULT_CONFIG(
                                                  lcd_handles->panel_handle,
                                                  lcd_handles->io_handle,
                                                  lcd_cfg->lcd_width,
                                                  lcd_cfg->lcd_height,
                                                  ESP_LV_ADAPTER_ROTATE_0);
    disp_cfg.tear_avoid_mode = tear_avoid_mode;
    disp_cfg.profile.use_psram = true;

    display_ = esp_lv_adapter_register_display(&disp_cfg);
    if (display_ == nullptr) {
        return fail_init("Register DSI display with LVGL adapter failed", ESP_ERR_NO_MEM);
    }
    if (lcd_cfg->frame_format == DEV_DISPLAY_LCD_FRAME_FORMAT_RGB565_BE) {
        lv_display_set_color_format(display_, LV_COLOR_FORMAT_RGB565_SWAPPED);
    }

    dev_lcd_touch_handles_t *touch_handles = nullptr;
    if (touch_device_initialized_ &&
            esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_LCD_TOUCH, (void **)&touch_handles) == ESP_OK &&
            touch_handles != nullptr) {
        const esp_lv_adapter_touch_config_t touch_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(
                                                          display_, touch_handles->touch_handle);
        if (esp_lv_adapter_register_touch(&touch_cfg) == nullptr) {
            ESP_LOGW(TAG, "Register touch failed, continue without touch");
        }
    }

    ret = esp_lv_adapter_start();
    if (ret != ESP_OK) {
        return fail_init("Start LVGL adapter failed", ret);
    }

    ESP_LOGI(TAG, "LCD %ux%u ready", lcd_cfg->lcd_width, lcd_cfg->lcd_height);
    return display_;
}

esp_err_t display_backlight_set(int percent)
{
    if (s_lcd_brightness_handle == nullptr) {
        esp_err_t ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_BRIGHTNESS);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Init lcd_brightness failed: %s", esp_err_to_name(ret));
            return ret;
        }

        void *brightness_handle = nullptr;
        ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_LCD_BRIGHTNESS, &brightness_handle);
        if (ret != ESP_OK || brightness_handle == nullptr) {
            const esp_err_t error = ret == ESP_OK ? ESP_ERR_INVALID_STATE : ret;
            ESP_LOGE(TAG, "Get lcd_brightness handle failed: %s", esp_err_to_name(error));
            const esp_err_t deinit_ret = esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_BRIGHTNESS);
            if (deinit_ret != ESP_OK) {
                ESP_LOGW(TAG, "Release lcd_brightness after init failure failed: %s", esp_err_to_name(deinit_ret));
            }
            return error;
        }
        s_lcd_brightness_handle = brightness_handle;
    }

    return custom_lcd_brightness_set(s_lcd_brightness_handle, std::clamp(percent, 0, 100));
}

void DisplayDevice::release_after_failed_init()
{
    if (adapter_initialized_) {
        (void)esp_lv_adapter_deinit();
        adapter_initialized_ = false;
    }
    display_ = nullptr;

    if (touch_device_initialized_) {
        (void)esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_TOUCH);
        touch_device_initialized_ = false;
    }
    if (display_device_initialized_) {
        (void)esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD);
        display_device_initialized_ = false;
    }
}

} // namespace esp_hal_boards
