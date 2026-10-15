/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "lvgl.h"
#include "systems/phone/esp_brookesia_phone_app.hpp"
#include "CameraPreview.hpp"

namespace esp_brookesia::apps {

/**
 * @brief Camera application: full-screen MIPI CSI preview
 *
 * The app owns an LVGL image and status label. CameraPreview produces frames in its own task and
 * publishes stable image descriptors for the GUI timer to consume. The worker never calls LVGL.
 */
class Camera : public systems::phone::App {
public:
    /**
     * @brief Get the singleton instance of Camera
     *
     * @param use_status_bar Show status bar
     * @param use_navigation_bar Show navigation bar
     * @return Camera* Singleton instance pointer
     */
    static Camera *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);

    ~Camera();

protected:
    Camera(bool use_status_bar, bool use_navigation_bar);

    bool run(void) override;
    bool back(void) override;
    bool close(void) override;
    bool init(void) override;
    bool deinit(void) override;
    bool pause(void) override;
    bool resume(void) override;

private:
    static Camera *_instance;

    static constexpr uint32_t OBSERVER_PERIOD_MS = 50;
    static constexpr uint32_t STOP_TIMEOUT_MS = 3000;

    /**
     * @brief Start the preview task and the observer timer that reacts to its state
     *
     * @return false if the task could not be created; the error is shown on the status label
     */
    bool startPreview();

    /**
     * @brief Stop the preview task, wait for it, hand the display back to LVGL (safe to call repeatedly)
     */
    bool stopPreview();

    void setStatusText(const char *text);
    void showError(esp_err_t err);
    void onObserverTick();

    static void observerTimerCb(lv_timer_t *timer);

    CameraPreview _preview;
    lv_display_t *_display = nullptr;
    lv_obj_t *_preview_image = nullptr;
    lv_obj_t *_status_label = nullptr;
    lv_timer_t *_observer_timer = nullptr;
};

} // namespace esp_brookesia::apps
