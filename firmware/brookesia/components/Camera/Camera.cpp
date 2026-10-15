/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <cinttypes>
#include "lvgl.h"
#include "esp_brookesia.hpp"
#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:Camera"
#include "esp_lib_utils.h"
#include "Camera.hpp"

#define APP_NAME "Camera"

using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems;

LV_IMG_DECLARE(img_app_camera);

namespace esp_brookesia::apps {

static constexpr const char *STATUS_STARTING = "Starting camera...";

Camera *Camera::_instance = nullptr;

Camera *Camera::requestInstance(bool use_status_bar, bool use_navigation_bar)
{
    if (_instance == nullptr) {
        _instance = new Camera(use_status_bar, use_navigation_bar);
    }
    return _instance;
}

Camera::Camera(bool use_status_bar, bool use_navigation_bar) :
    App(APP_NAME, &img_app_camera, true, use_status_bar, use_navigation_bar)
{
}

Camera::~Camera()
{
    // LVGL objects are owned by the app screen; CameraPreview stops its worker in its destructor.
}

bool Camera::run(void)
{
    ESP_UTILS_LOGD("Run");

    _display = lv_display_get_default();
    ESP_UTILS_CHECK_NULL_RETURN(_display, false, "No default display");

    lv_obj_t *screen = lv_scr_act();
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    _preview_image = lv_image_create(screen);
    ESP_UTILS_CHECK_NULL_RETURN(_preview_image, false, "Create preview image failed");
    lv_obj_add_flag(_preview_image, LV_OBJ_FLAG_HIDDEN);

    const lv_area_t &area = getVisualArea();
    _status_label = lv_label_create(screen);
    ESP_UTILS_CHECK_NULL_RETURN(_status_label, false, "Create status label failed");
    lv_obj_set_width(_status_label, lv_area_get_width(&area));
    lv_label_set_long_mode(_status_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(_status_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(_status_label, &lv_font_montserrat_30, LV_PART_MAIN);
    lv_obj_set_style_text_color(_status_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(_status_label, LV_ALIGN_CENTER, 0, 0);
    setStatusText(STATUS_STARTING);

    // A failed start is reported on the label; the app still opens so the user sees why
    startPreview();

    return true;
}

bool Camera::back(void)
{
    ESP_UTILS_LOGD("Back");

    ESP_UTILS_CHECK_FALSE_RETURN(notifyCoreClosed(), false, "Notify core closed failed");

    return true;
}

bool Camera::close(void)
{
    ESP_UTILS_LOGD("Close");

    if (!stopPreview()) {
        return false;
    }
    // The core deletes the screen and its objects.
    _preview_image = nullptr;
    _status_label = nullptr;

    return true;
}

bool Camera::init(void)
{
    ESP_UTILS_LOGD("Init");

    return true;
}

bool Camera::deinit(void)
{
    ESP_UTILS_LOGD("Deinit");

    return stopPreview();
}

bool Camera::pause(void)
{
    ESP_UTILS_LOGD("Pause");

    return stopPreview();
}

bool Camera::resume(void)
{
    ESP_UTILS_LOGD("Resume");

    setStatusText(STATUS_STARTING);
    startPreview();

    return true;
}

bool Camera::startPreview()
{
    if (_observer_timer != nullptr) {
        if (!stopPreview()) {
            return false;
        }
    }
    esp_err_t ret = _preview.start(_display);
    if (ret != ESP_OK) {
        _preview.releaseFrames();
        showError(ret);
        return false;
    }

    _observer_timer = lv_timer_create(observerTimerCb, OBSERVER_PERIOD_MS, this);
    if (_observer_timer == nullptr) {
        ESP_UTILS_LOGE("Create observer timer failed");
        stopPreview();
        showError(ESP_ERR_NO_MEM);
        return false;
    }

    return true;
}

bool Camera::stopPreview()
{
    // Clear the LVGL source before releasing its PSRAM frame buffers.
    if (_preview_image != nullptr) {
        lv_image_set_src(_preview_image, nullptr);
        lv_obj_add_flag(_preview_image, LV_OBJ_FLAG_HIDDEN);
    }
    if (_observer_timer != nullptr) {
        lv_timer_pause(_observer_timer);
    }

    _preview.requestStop();
    if (!_preview.waitStopped(STOP_TIMEOUT_MS)) {
        // The worker may still be using its frame buffers; retry after it exits.
        ESP_UTILS_LOGE("Preview task did not stop within %" PRIu32 " ms", STOP_TIMEOUT_MS);
        return false;
    }
    _preview.releaseFrames();

    if (_observer_timer != nullptr) {
        lv_timer_delete(_observer_timer);
        _observer_timer = nullptr;
    }
    return true;
}

void Camera::setStatusText(const char *text)
{
    if (_status_label != nullptr) {
        lv_label_set_text(_status_label, text);
        lv_obj_remove_flag(_status_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(_status_label);
    }
    if (_preview_image != nullptr) {
        lv_obj_add_flag(_preview_image, LV_OBJ_FLAG_HIDDEN);
    }
}

void Camera::showError(esp_err_t err)
{
    ESP_UTILS_LOGE("Camera unavailable: %s", esp_err_to_name(err));
    if (_preview_image != nullptr) {
        lv_obj_add_flag(_preview_image, LV_OBJ_FLAG_HIDDEN);
    }
    if (_status_label != nullptr) {
        lv_label_set_text_fmt(_status_label, "Camera unavailable\n%s", esp_err_to_name(err));
        lv_obj_remove_flag(_status_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(_status_label);
    }
}

void Camera::onObserverTick()
{
    switch (_preview.getState()) {
    case CameraPreview::State::Ready:
        _preview.setDisplayReady();
        break;
    case CameraPreview::State::Running: {
        const lv_image_dsc_t *frame = _preview.takeLatestFrame();
        if (frame != nullptr && _preview_image != nullptr) {
            lv_image_set_src(_preview_image, frame);
            lv_obj_center(_preview_image);
            lv_obj_remove_flag(_preview_image, LV_OBJ_FLAG_HIDDEN);
            if (_status_label != nullptr) {
                lv_obj_add_flag(_status_label, LV_OBJ_FLAG_HIDDEN);
            }
        }
        break;
    }
    case CameraPreview::State::Error:
    case CameraPreview::State::Stopped: {
        // Nobody asked it to stop, so the task exited on its own.
        const esp_err_t err = _preview.getError();
        stopPreview();
        showError(err == ESP_OK ? ESP_ERR_INVALID_STATE : err);
        break;
    }
    default:
        break;
    }
}

void Camera::observerTimerCb(lv_timer_t *timer)
{
    Camera *camera = static_cast<Camera *>(lv_timer_get_user_data(timer));
    ESP_UTILS_CHECK_NULL_EXIT(camera, "Invalid camera");
    camera->onObserverTick();
}

} // namespace esp_brookesia::apps
