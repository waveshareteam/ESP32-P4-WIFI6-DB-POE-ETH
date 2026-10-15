/*
 * SPDX-FileCopyrightText: 2026 Waveshare Electronics
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_err.h"
#include "lvgl.h"

namespace esp_hal_boards {

/** Board Manager-backed LCD and LVGL adapter initialization. */
class DisplayDevice {
public:
    DisplayDevice(const DisplayDevice &) = delete;
    DisplayDevice &operator=(const DisplayDevice &) = delete;
    DisplayDevice(DisplayDevice &&) = delete;
    DisplayDevice &operator=(DisplayDevice &&) = delete;

    static DisplayDevice &get_instance();

    /**
     * Initialize the configured LCD, optional touch device, and LVGL adapter.
     * Repeated calls return the already registered LVGL display.
     *
     * @return The LVGL display, or nullptr on failure.
     */
    lv_display_t *init();

private:
    DisplayDevice() = default;

    void release_after_failed_init();

    bool display_device_initialized_ = false;
    bool touch_device_initialized_ = false;
    bool adapter_initialized_ = false;
    lv_display_t *display_ = nullptr;
};

/**
 * Set the LCD backlight brightness through the configured Board Manager device.
 * The brightness device is initialized on the first call.
 *
 * @param[in] percent Brightness percentage; values are clamped to 0-100.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t display_backlight_set(int percent);

/** Load the saved LCD backlight brightness percentage from persistent storage. */
int display_backlight_get_saved();

} // namespace esp_hal_boards
