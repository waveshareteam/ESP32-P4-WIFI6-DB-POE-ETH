/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "driver/ppa.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"

namespace esp_brookesia::apps {

/**
 * @brief Full-screen camera preview pipeline
 *
 * MIPI CSI (V4L2, MMAP buffers) -> PPA aspect-fit scaling into PSRAM buffers -> LVGL image object.
 * The capture pipeline runs on its own task and never calls LVGL. The GUI task consumes the latest
 * completed frame descriptor from its observer timer.
 *
 * Expected usage from the GUI task:
 *   start() -> poll getState() until Ready -> setDisplayReady() -> takeLatestFrame()
 *   clear the LVGL image source -> requestStop() -> waitStopped() -> releaseFrames()
 */
class CameraPreview {
public:
    enum class State : uint8_t {
        Idle,       /*!< No task running, never started or fully cleaned up */
        Starting,   /*!< Task is bringing up the camera device and V4L2 stream */
        Ready,      /*!< Stream is on; waiting for the GUI task to enable frame publishing */
        Running,    /*!< Frames are being produced for the LVGL image */
        Stopped,    /*!< Task exited on request and released task resources */
        Error,      /*!< Task exited because of an error; see getError() */
    };

    CameraPreview();
    ~CameraPreview();

    CameraPreview(const CameraPreview &) = delete;
    CameraPreview &operator=(const CameraPreview &) = delete;

    /**
     * @brief Create the preview task and return immediately
     *
     * @param display LVGL display used to select frame dimensions and color format
     * @return ESP_OK, or ESP_ERR_INVALID_STATE if a previous task is still alive
     */
    esp_err_t start(lv_display_t *display);

    /**
     * @brief Ask the task to stop; returns immediately
     */
    void requestStop();

    /**
     * @brief Block until the task has exited and released its resources
     *
     * @return true if the task is gone, false on timeout (resources are still owned by the task)
     */
    bool waitStopped(uint32_t timeout_ms);

    /**
     * @brief Tell the task that the GUI is ready to consume frames
     */
    void setDisplayReady();

    /**
     * @brief Take the latest completed frame for display, or nullptr if none is ready
     *
     * The returned descriptor remains valid until releaseFrames() and is safe for LVGL to read
     * until a later call to takeLatestFrame() replaces it.
     */
    const lv_image_dsc_t *takeLatestFrame();

    /**
     * @brief Release the PSRAM frame buffers after the worker has stopped and LVGL dropped its source
     */
    void releaseFrames();

    State getState() const
    {
        return _state.load(std::memory_order_acquire);
    }

    esp_err_t getError() const
    {
        return _error.load(std::memory_order_acquire);
    }

    bool isTaskAlive() const
    {
        return _task_alive.load(std::memory_order_acquire);
    }

private:
    static constexpr int BUFFER_COUNT = 3;
    static constexpr int FRAME_BUFFER_COUNT = 2;
    static constexpr uint32_t TASK_STACK_SIZE = 8 * 1024;
    static constexpr UBaseType_t TASK_PRIORITY = 6;
    static constexpr BaseType_t TASK_CORE = 0;
    static constexpr uint32_t DQBUF_TIMEOUT_MS = 500;
    static constexpr uint32_t MAX_CONSECUTIVE_DQBUF_TIMEOUTS = 4;
    static constexpr uint32_t DISPLAY_READY_POLL_MS = 10;
    static constexpr uint32_t DISPLAY_READY_TIMEOUT_MS = 2000;
    static constexpr uint32_t FPS_LOG_PERIOD_MS = 5000;

    struct MappedBuffer {
        void *data = nullptr;
        size_t length = 0;
    };

    enum class FrameState : uint8_t {
        Free,
        Writing,
        Ready,
        Displayed,
    };

    struct PreviewFrame {
        void *data = nullptr;
        lv_image_dsc_t image = {};
        FrameState state = FrameState::Free;
    };

    esp_err_t initCameraDevice();
    esp_err_t openDevice();
    esp_err_t configureFormat();
    esp_err_t allocateFrames();
    esp_err_t setupBuffers();
    esp_err_t setupPpa();
    esp_err_t streamOn();
    bool waitDisplayReady();
    esp_err_t pumpFrame();
    int acquireFrame();
    void releaseWritingFrame(int index);
    void publishFrame(int index);
    void teardown();
    void taskMain();
    static void taskEntry(void *arg);

    bool stopRequested() const
    {
        return _stop_requested.load(std::memory_order_acquire);
    }

    void setState(State state)
    {
        _state.store(state, std::memory_order_release);
    }

    lv_display_t *_display = nullptr;
    StaticSemaphore_t _done_sem_storage = {};
    SemaphoreHandle_t _done_sem = nullptr;

    std::atomic<bool> _task_alive{false};
    std::atomic<State> _state{State::Idle};
    std::atomic<esp_err_t> _error{ESP_OK};
    std::atomic<bool> _stop_requested{false};
    std::atomic<bool> _display_ready{false};

    portMUX_TYPE _frame_mux = portMUX_INITIALIZER_UNLOCKED;
    PreviewFrame _frames[FRAME_BUFFER_COUNT] = {};
    int _ready_frame_index = -1;
    int _display_frame_index = -1;

    // Everything below is owned by the preview task
    int _fd = -1;
    bool _streaming = false;
    ppa_client_handle_t _ppa = nullptr;
    MappedBuffer _buffers[BUFFER_COUNT] = {};
    uint32_t _buffer_count = 0;
    uint32_t _cam_width = 0;
    uint32_t _cam_height = 0;
    uint32_t _preview_width = 0;
    uint32_t _preview_height = 0;
    float _preview_scale = 1.0f;
    uint32_t _disp_width = 0;
    uint32_t _disp_height = 0;
    uint32_t _bytes_per_pixel = 0;
    uint32_t _v4l2_pixel_format = 0;
    ppa_srm_color_mode_t _ppa_color_mode = PPA_SRM_COLOR_MODE_RGB565;
    lv_color_format_t _lv_color_format = LV_COLOR_FORMAT_RGB565;
    size_t _frame_buffer_size = 0;
};

} // namespace esp_brookesia::apps
