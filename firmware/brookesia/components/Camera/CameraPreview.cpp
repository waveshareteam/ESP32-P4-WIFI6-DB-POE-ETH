/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <cerrno>
#include <cinttypes>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sdkconfig.h"

#include "dev_camera.h"
#include "esp_board_manager.h"
#include "esp_board_manager_defs.h"
#include "esp_video_ioctl.h"
#include "linux/videodev2.h"

#include "CameraPreview.hpp"

static const char *TAG = "CameraPreview";

#ifdef CONFIG_CACHE_L2_CACHE_LINE_SIZE
#define CACHE_LINE_SIZE CONFIG_CACHE_L2_CACHE_LINE_SIZE
#else
#define CACHE_LINE_SIZE 64
#endif

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

namespace esp_brookesia::apps {

CameraPreview::CameraPreview()
{
    _done_sem = xSemaphoreCreateBinaryStatic(&_done_sem_storage);
}

CameraPreview::~CameraPreview()
{
    requestStop();
    // Keep this object alive until the worker has made its last access.
    while (!waitStopped(1000)) {
        ESP_LOGW(TAG, "Waiting for preview task before destruction");
    }
    releaseFrames();
    vSemaphoreDelete(_done_sem);
}

esp_err_t CameraPreview::start(lv_display_t *display)
{
    ESP_RETURN_ON_FALSE(display != nullptr, ESP_ERR_INVALID_ARG, TAG, "Display is NULL");
    ESP_RETURN_ON_FALSE(waitStopped(0), ESP_ERR_INVALID_STATE, TAG, "Previous preview task is still alive");

    _display = display;
    _disp_width = lv_display_get_horizontal_resolution(_display);
    _disp_height = lv_display_get_vertical_resolution(_display);

    switch (lv_display_get_color_format(_display)) {
    case LV_COLOR_FORMAT_RGB565:
        _v4l2_pixel_format = V4L2_PIX_FMT_RGB565;
        _ppa_color_mode = PPA_SRM_COLOR_MODE_RGB565;
        _lv_color_format = LV_COLOR_FORMAT_RGB565;
        _bytes_per_pixel = 2;
        break;
    case LV_COLOR_FORMAT_RGB888:
        _v4l2_pixel_format = V4L2_PIX_FMT_RGB24;
        _ppa_color_mode = PPA_SRM_COLOR_MODE_RGB888;
        _lv_color_format = LV_COLOR_FORMAT_RGB888;
        _bytes_per_pixel = 3;
        break;
    default:
        ESP_LOGE(TAG, "Unsupported display color format %d", lv_display_get_color_format(_display));
        return ESP_ERR_NOT_SUPPORTED;
    }

    _stop_requested.store(false, std::memory_order_release);
    _display_ready.store(false, std::memory_order_release);
    _error.store(ESP_OK, std::memory_order_release);
    setState(State::Starting);

    // Drop a completion left over from a task nobody waited for
    xSemaphoreTake(_done_sem, 0);

    // Only the GUI waiter clears this after consuming the completion signal.
    _task_alive.store(true, std::memory_order_release);
    BaseType_t ret = xTaskCreatePinnedToCore(
                         taskEntry, "camera_preview", TASK_STACK_SIZE, this, TASK_PRIORITY, nullptr, TASK_CORE
                     );
    if (ret != pdPASS) {
        _task_alive.store(false, std::memory_order_release);
        _error.store(ESP_ERR_NO_MEM, std::memory_order_release);
        setState(State::Error);
        ESP_LOGE(TAG, "Create preview task failed");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

void CameraPreview::requestStop()
{
    _stop_requested.store(true, std::memory_order_release);
}

bool CameraPreview::waitStopped(uint32_t timeout_ms)
{
    if (!isTaskAlive()) {
        return true;
    }
    if (xSemaphoreTake(_done_sem, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) {
        _task_alive.store(false, std::memory_order_release);
        return true;
    }
    return false;
}

void CameraPreview::setDisplayReady()
{
    _display_ready.store(true, std::memory_order_release);
}

esp_err_t CameraPreview::initCameraDevice()
{
    // esp_video stays initialized for the rest of the boot: re-probing the sensor on every
    // launch is slow and esp_video init/deinit cycles are the least exercised path.
    static bool s_device_ready = false;

    if (s_device_ready) {
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_CAMERA), TAG,
                        "Init camera device failed");
    s_device_ready = true;
    return ESP_OK;
}

esp_err_t CameraPreview::openDevice()
{
    dev_camera_handle_t *handle = nullptr;
    ESP_RETURN_ON_ERROR(esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_CAMERA, (void **)&handle), TAG,
                        "Get camera device handle failed");
    ESP_RETURN_ON_FALSE(handle != nullptr && handle->dev_path != nullptr, ESP_ERR_INVALID_STATE, TAG,
                        "Camera device path is NULL");

    _fd = open(handle->dev_path, O_RDONLY);
    ESP_RETURN_ON_FALSE(_fd >= 0, ESP_FAIL, TAG, "Open %s failed (errno %d)", handle->dev_path, errno);

    struct v4l2_capability capability = {};
    ESP_RETURN_ON_FALSE(ioctl(_fd, VIDIOC_QUERYCAP, &capability) == 0, ESP_FAIL, TAG, "VIDIOC_QUERYCAP failed");
    ESP_RETURN_ON_FALSE(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE, ESP_ERR_NOT_SUPPORTED, TAG,
                        "%s is not a capture device", handle->dev_path);

    // Bound DQBUF so the loop can always observe a stop request even if the sensor stops delivering
    struct timeval timeout = {};
    timeout.tv_sec = 0;
    timeout.tv_usec = DQBUF_TIMEOUT_MS * 1000;
    ESP_RETURN_ON_FALSE(ioctl(_fd, VIDIOC_S_DQBUF_TIMEOUT, &timeout) == 0, ESP_FAIL, TAG,
                        "VIDIOC_S_DQBUF_TIMEOUT failed");

    return ESP_OK;
}

esp_err_t CameraPreview::configureFormat()
{
    struct v4l2_format format = {};
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ESP_RETURN_ON_FALSE(ioctl(_fd, VIDIOC_G_FMT, &format) == 0, ESP_FAIL, TAG, "VIDIOC_G_FMT failed");

    // The CSI node keeps the sensor mode's width/height and rejects changes to them, so only
    // the pixel format is requested when it differs from the display's.
    if (format.fmt.pix.pixelformat != _v4l2_pixel_format) {
        format.fmt.pix.pixelformat = _v4l2_pixel_format;
        ESP_RETURN_ON_FALSE(ioctl(_fd, VIDIOC_S_FMT, &format) == 0, ESP_FAIL, TAG,
                            "VIDIOC_S_FMT to display pixel format failed");
        format = {};
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ESP_RETURN_ON_FALSE(ioctl(_fd, VIDIOC_G_FMT, &format) == 0, ESP_FAIL, TAG, "VIDIOC_G_FMT failed");
        ESP_RETURN_ON_FALSE(format.fmt.pix.pixelformat == _v4l2_pixel_format, ESP_ERR_NOT_SUPPORTED, TAG,
                            "Camera pixel format 0x%08" PRIx32 " does not match the display",
                            format.fmt.pix.pixelformat);
    }

    _cam_width = format.fmt.pix.width;
    _cam_height = format.fmt.pix.height;
    ESP_RETURN_ON_FALSE(_cam_width > 0 && _cam_height > 0, ESP_ERR_INVALID_SIZE, TAG, "Invalid camera size");
    ESP_RETURN_ON_FALSE(_cam_width <= UINT16_MAX && _cam_height <= UINT16_MAX,
                        ESP_ERR_NOT_SUPPORTED, TAG, "Camera dimensions exceed LVGL image limits");

    // Keep small frames at 1:1 and shrink larger frames without cropping. Quantize down
    // to 1/16, which is representable by PPA on all supported P4 revisions.
    uint32_t scale_steps = 16;
    const uint32_t width_steps = static_cast<uint32_t>((uint64_t)_disp_width * 16 / _cam_width);
    const uint32_t height_steps = static_cast<uint32_t>((uint64_t)_disp_height * 16 / _cam_height);
    if (width_steps < scale_steps) {
        scale_steps = width_steps;
    }
    if (height_steps < scale_steps) {
        scale_steps = height_steps;
    }
    ESP_RETURN_ON_FALSE(scale_steps > 0, ESP_ERR_NOT_SUPPORTED, TAG, "Camera requires scaling below 1/16");
    _preview_scale = scale_steps / 16.0f;
    _preview_width = static_cast<uint32_t>((uint64_t)_cam_width * scale_steps / 16);
    _preview_height = static_cast<uint32_t>((uint64_t)_cam_height * scale_steps / 16);
    ESP_RETURN_ON_FALSE(_preview_width > 0 && _preview_height > 0,
                        ESP_ERR_INVALID_SIZE, TAG, "Invalid preview size");

    const uint32_t tight_stride = _cam_width * _bytes_per_pixel;
    ESP_RETURN_ON_FALSE(tight_stride <= UINT16_MAX, ESP_ERR_NOT_SUPPORTED, TAG,
                        "Camera stride exceeds LVGL image limits");
    ESP_RETURN_ON_FALSE(format.fmt.pix.bytesperline == 0 || format.fmt.pix.bytesperline == tight_stride,
                        ESP_ERR_NOT_SUPPORTED, TAG, "Padded camera stride %" PRIu32 " is not supported",
                        format.fmt.pix.bytesperline);

    ESP_RETURN_ON_FALSE(_preview_width * _bytes_per_pixel <= UINT16_MAX,
                        ESP_ERR_NOT_SUPPORTED, TAG, "Preview stride exceeds LVGL image limits");
    const uint64_t data_size = (uint64_t)_preview_width * _preview_height * _bytes_per_pixel;
    ESP_RETURN_ON_FALSE(data_size <= SIZE_MAX - (CACHE_LINE_SIZE - 1), ESP_ERR_INVALID_SIZE, TAG,
                        "Camera frame size is too large");
    _frame_buffer_size = ALIGN_UP(static_cast<size_t>(data_size), CACHE_LINE_SIZE);
    ESP_LOGI(TAG, "Camera %" PRIu32 "x%" PRIu32 " -> preview %" PRIu32 "x%" PRIu32 " (scale %.4f)",
             _cam_width, _cam_height, _preview_width, _preview_height, _preview_scale);

    return ESP_OK;
}

esp_err_t CameraPreview::allocateFrames()
{
    const uint64_t data_size_64 = (uint64_t)_preview_width * _preview_height * _bytes_per_pixel;
    const uint32_t stride = _preview_width * _bytes_per_pixel;
    if (data_size_64 > UINT32_MAX) {
        return ESP_ERR_INVALID_SIZE;
    }
    const uint32_t data_size = static_cast<uint32_t>(data_size_64);

    for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {
        void *data = heap_caps_aligned_alloc(CACHE_LINE_SIZE, _frame_buffer_size,
                                             MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (data == nullptr) {
            for (int allocated = 0; allocated < i; allocated++) {
                heap_caps_free(_frames[allocated].data);
                _frames[allocated] = {};
            }
            return ESP_ERR_NO_MEM;
        }

        _frames[i].data = data;
        _frames[i].image = {};
        _frames[i].image.header.magic = LV_IMAGE_HEADER_MAGIC;
        _frames[i].image.header.cf = _lv_color_format;
        _frames[i].image.header.w = static_cast<uint16_t>(_preview_width);
        _frames[i].image.header.h = static_cast<uint16_t>(_preview_height);
        _frames[i].image.header.stride = static_cast<uint16_t>(stride);
        _frames[i].image.data_size = data_size;
        _frames[i].image.data = static_cast<const uint8_t *>(data);
        _frames[i].state = FrameState::Free;
    }

    portENTER_CRITICAL(&_frame_mux);
    _ready_frame_index = -1;
    _display_frame_index = -1;
    portEXIT_CRITICAL(&_frame_mux);
    return ESP_OK;
}

void CameraPreview::releaseFrames()
{
    if (isTaskAlive()) {
        ESP_LOGW(TAG, "Cannot release preview frames while the worker is running");
        return;
    }

    void *buffers[FRAME_BUFFER_COUNT] = {};
    portENTER_CRITICAL(&_frame_mux);
    for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {
        buffers[i] = _frames[i].data;
        _frames[i] = {};
    }
    _ready_frame_index = -1;
    _display_frame_index = -1;
    portEXIT_CRITICAL(&_frame_mux);

    for (void *buffer : buffers) {
        if (buffer != nullptr) {
            heap_caps_free(buffer);
        }
    }
}

int CameraPreview::acquireFrame()
{
    int index = -1;
    portENTER_CRITICAL(&_frame_mux);
    for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {
        if (_frames[i].data != nullptr && _frames[i].state == FrameState::Free) {
            _frames[i].state = FrameState::Writing;
            index = i;
            break;
        }
    }
    portEXIT_CRITICAL(&_frame_mux);
    return index;
}

void CameraPreview::releaseWritingFrame(int index)
{
    if (index < 0 || index >= FRAME_BUFFER_COUNT) {
        return;
    }
    portENTER_CRITICAL(&_frame_mux);
    if (_frames[index].state == FrameState::Writing) {
        _frames[index].state = FrameState::Free;
    }
    portEXIT_CRITICAL(&_frame_mux);
}

void CameraPreview::publishFrame(int index)
{
    if (index < 0 || index >= FRAME_BUFFER_COUNT) {
        return;
    }
    portENTER_CRITICAL(&_frame_mux);
    if (_frames[index].state == FrameState::Writing) {
        if (_ready_frame_index >= 0) {
            _frames[_ready_frame_index].state = FrameState::Free;
        }
        _frames[index].state = FrameState::Ready;
        _ready_frame_index = index;
    }
    portEXIT_CRITICAL(&_frame_mux);
}

const lv_image_dsc_t *CameraPreview::takeLatestFrame()
{
    const lv_image_dsc_t *image = nullptr;
    portENTER_CRITICAL(&_frame_mux);
    if (_ready_frame_index >= 0) {
        const int next_index = _ready_frame_index;
        if (_display_frame_index >= 0) {
            _frames[_display_frame_index].state = FrameState::Free;
        }
        _frames[next_index].state = FrameState::Displayed;
        _display_frame_index = next_index;
        _ready_frame_index = -1;
        image = &_frames[next_index].image;
    }
    portEXIT_CRITICAL(&_frame_mux);
    return image;
}

esp_err_t CameraPreview::setupBuffers()
{
    struct v4l2_requestbuffers req = {};
    req.count = BUFFER_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    ESP_RETURN_ON_FALSE(ioctl(_fd, VIDIOC_REQBUFS, &req) == 0, ESP_FAIL, TAG, "VIDIOC_REQBUFS failed");
    // One buffer is held by this task during the PPA copy, so the driver needs at least one more
    ESP_RETURN_ON_FALSE(req.count >= 2 && req.count <= BUFFER_COUNT, ESP_ERR_NO_MEM, TAG,
                        "Camera returned %" PRIu32 " buffers", req.count);
    _buffer_count = req.count;

    for (uint32_t i = 0; i < _buffer_count; i++) {
        struct v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        ESP_RETURN_ON_FALSE(ioctl(_fd, VIDIOC_QUERYBUF, &buf) == 0, ESP_FAIL, TAG, "VIDIOC_QUERYBUF %" PRIu32 " failed", i);

        ESP_RETURN_ON_FALSE(buf.length >= (size_t)_cam_width * _cam_height * _bytes_per_pixel,
                            ESP_ERR_INVALID_SIZE, TAG, "Camera buffer is too small");
        void *data = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, buf.m.offset);
        ESP_RETURN_ON_FALSE(data != MAP_FAILED, ESP_FAIL, TAG, "mmap buffer %" PRIu32 " failed", i);
        _buffers[i].data = data;
        _buffers[i].length = buf.length;

        ESP_RETURN_ON_FALSE(ioctl(_fd, VIDIOC_QBUF, &buf) == 0, ESP_FAIL, TAG, "VIDIOC_QBUF %" PRIu32 " failed", i);
    }

    return ESP_OK;
}

esp_err_t CameraPreview::setupPpa()
{
    ppa_client_config_t config = {};
    config.oper_type = PPA_OPERATION_SRM;
    ESP_RETURN_ON_ERROR(ppa_register_client(&config, &_ppa), TAG, "Register PPA SRM client failed");
    return ESP_OK;
}

esp_err_t CameraPreview::streamOn()
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ESP_RETURN_ON_FALSE(ioctl(_fd, VIDIOC_STREAMON, &type) == 0, ESP_FAIL, TAG, "VIDIOC_STREAMON failed");
    _streaming = true;
    return ESP_OK;
}

bool CameraPreview::waitDisplayReady()
{
    const int64_t deadline_us = esp_timer_get_time() + (int64_t)DISPLAY_READY_TIMEOUT_MS * 1000;

    while (!stopRequested()) {
        if (_display_ready.load(std::memory_order_acquire)) {
            return true;
        }
        if (esp_timer_get_time() >= deadline_us) {
            ESP_LOGE(TAG, "Display was not ready within %" PRIu32 " ms", DISPLAY_READY_TIMEOUT_MS);
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_READY_POLL_MS));
    }
    return false;
}

esp_err_t CameraPreview::pumpFrame()
{
    struct v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(_fd, VIDIOC_DQBUF, &buf) != 0) {
        if (errno == ETIMEDOUT) {
            return ESP_ERR_TIMEOUT;
        }
        ESP_LOGE(TAG, "VIDIOC_DQBUF failed (errno %d)", errno);
        return ESP_FAIL;
    }

    ESP_RETURN_ON_FALSE(buf.index < _buffer_count, ESP_ERR_INVALID_RESPONSE, TAG, "Invalid buffer index");
    const bool frame_valid = (buf.flags & V4L2_BUF_FLAG_DONE) && !(buf.flags & V4L2_BUF_FLAG_ERROR) &&
                             buf.bytesused <= _buffers[buf.index].length &&
                             buf.bytesused >= (size_t)_cam_width * _cam_height * _bytes_per_pixel;

    const int frame_index = frame_valid ? acquireFrame() : -1;
    esp_err_t ret = ESP_OK;
    if (!frame_valid) {
        ret = ESP_ERR_INVALID_SIZE;
    } else if (frame_index >= 0) {
        ppa_srm_oper_config_t srm = {};
        srm.in.buffer = _buffers[buf.index].data;
        srm.in.pic_w = _cam_width;
        srm.in.pic_h = _cam_height;
        srm.in.block_w = _cam_width;
        srm.in.block_h = _cam_height;
        srm.in.block_offset_x = 0;
        srm.in.block_offset_y = 0;
        srm.in.srm_cm = _ppa_color_mode;
        srm.out.buffer = _frames[frame_index].data;
        srm.out.buffer_size = _frame_buffer_size;
        srm.out.pic_w = _preview_width;
        srm.out.pic_h = _preview_height;
        srm.out.block_offset_x = 0;
        srm.out.block_offset_y = 0;
        srm.out.srm_cm = _ppa_color_mode;
        srm.rotation_angle = PPA_SRM_ROTATION_ANGLE_0;
        srm.scale_x = _preview_scale;
        srm.scale_y = _preview_scale;
        srm.mode = PPA_TRANS_MODE_BLOCKING;
        ret = ppa_do_scale_rotate_mirror(_ppa, &srm);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "PPA scaling failed: %s", esp_err_to_name(ret));
        }
    }

    // Hand the buffer back before waiting for the display so the sensor never starves
    if (ioctl(_fd, VIDIOC_QBUF, &buf) != 0) {
        releaseWritingFrame(frame_index);
        ESP_LOGE(TAG, "VIDIOC_QBUF failed (errno %d)", errno);
        return ESP_FAIL;
    }
    if (ret != ESP_OK) {
        releaseWritingFrame(frame_index);
        return ret;
    }
    if (stopRequested()) {
        releaseWritingFrame(frame_index);
        return ESP_OK;
    }
    if (frame_index >= 0) {
        publishFrame(frame_index);
    }
    return ESP_OK;
}

void CameraPreview::teardown()
{
    if (_streaming) {
        int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(_fd, VIDIOC_STREAMOFF, &type) != 0) {
            ESP_LOGW(TAG, "VIDIOC_STREAMOFF failed (errno %d)", errno);
        }
        _streaming = false;
    }

    for (uint32_t i = 0; i < BUFFER_COUNT; i++) {
        if (_buffers[i].data != nullptr) {
            munmap(_buffers[i].data, _buffers[i].length);
            _buffers[i] = {};
        }
    }
    _buffer_count = 0;

    if (_ppa != nullptr) {
        ppa_unregister_client(_ppa);
        _ppa = nullptr;
    }

    if (_fd >= 0) {
        close(_fd);
        _fd = -1;
    }
}

void CameraPreview::taskMain()
{
    esp_err_t ret = ESP_OK;

    setState(State::Starting);

    esp_err_t (CameraPreview::*const setup_steps[])() = {
        &CameraPreview::initCameraDevice,
        &CameraPreview::openDevice,
        &CameraPreview::configureFormat,
        &CameraPreview::allocateFrames,
        &CameraPreview::setupBuffers,
        &CameraPreview::setupPpa,
        &CameraPreview::streamOn,
    };
    for (auto step : setup_steps) {
        if (stopRequested()) {
            break;
        }
        ret = (this->*step)();
        if (ret != ESP_OK) {
            break;
        }
    }

    if (ret == ESP_OK && !stopRequested()) {
        setState(State::Ready);
        if (!waitDisplayReady()) {
            ret = stopRequested() ? ESP_OK : ESP_ERR_TIMEOUT;
        }
    }

    if (ret == ESP_OK && !stopRequested()) {
        setState(State::Running);
        ESP_LOGI(TAG, "Preview %" PRIu32 "x%" PRIu32 " -> display %" PRIu32 "x%" PRIu32 ", %" PRIu32 " buffers",
                 _cam_width, _cam_height, _disp_width, _disp_height, _buffer_count);

        uint32_t consecutive_timeouts = 0;
        uint32_t frames = 0;
        int64_t fps_window_start_us = esp_timer_get_time();

        while (!stopRequested()) {
            ret = pumpFrame();
            if (ret == ESP_ERR_TIMEOUT) {
                if (++consecutive_timeouts >= MAX_CONSECUTIVE_DQBUF_TIMEOUTS) {
                    ESP_LOGE(TAG, "No camera frame for %" PRIu32 " ms",
                             consecutive_timeouts * DQBUF_TIMEOUT_MS);
                    break;
                }
                continue;
            }
            if (ret != ESP_OK) {
                break;
            }
            consecutive_timeouts = 0;

            frames++;
            const int64_t now_us = esp_timer_get_time();
            const int64_t elapsed_us = now_us - fps_window_start_us;
            if (elapsed_us >= (int64_t)FPS_LOG_PERIOD_MS * 1000) {
                ESP_LOGD(TAG, "Preview %.1f fps", (double)frames * 1e6 / (double)elapsed_us);
                frames = 0;
                fps_window_start_us = now_us;
            }
        }
        if (ret == ESP_ERR_TIMEOUT && stopRequested()) {
            // A stop request that lands during a DQBUF timeout is a normal exit
            ret = ESP_OK;
        }
    }

    teardown();

    _error.store(ret, std::memory_order_release);
    setState(ret == ESP_OK ? State::Stopped : State::Error);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Preview stopped: %s", esp_err_to_name(ret));
    }

    // Final access to this object; the GUI can destroy it after taking this signal.
    xSemaphoreGive(_done_sem);
    vTaskDelete(nullptr);
}

void CameraPreview::taskEntry(void *arg)
{
    static_cast<CameraPreview *>(arg)->taskMain();
}

} // namespace esp_brookesia::apps
