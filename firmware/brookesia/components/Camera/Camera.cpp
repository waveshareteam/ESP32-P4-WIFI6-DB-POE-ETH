/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "Camera.hpp"
#include "lvgl.h"
#include "esp_brookesia.hpp"

#include "bsp/touch.h"
#include "esp_cache.h"
#include "esp_lib_utils.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "esp_video_device.h"
#include "esp_video_init.h"
#include "linux/videodev2.h"

#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:Camera"

LV_IMG_DECLARE(img_app_camera);

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

#if CONFIG_BSP_LCD_COLOR_FORMAT_RGB565
#define CAMERA_VIDEO_FMT V4L2_PIX_FMT_RGB565
#define CAMERA_PPA_COLOR_MODE PPA_SRM_COLOR_MODE_RGB565
#define CAMERA_BYTES_PER_PIXEL 2
#elif CONFIG_BSP_LCD_COLOR_FORMAT_RGB888
#define CAMERA_VIDEO_FMT V4L2_PIX_FMT_RGB24
#define CAMERA_PPA_COLOR_MODE PPA_SRM_COLOR_MODE_RGB888
#define CAMERA_BYTES_PER_PIXEL 3
#else
#error "Unsupported BSP LCD color format"
#endif


namespace esp_brookesia::apps
{
    // The OV5647 runs its 800x640 binned mode (the 800x1280 mode has a much
    // stronger colour cast) and the PPA cover-fills the LCD panel with it. A
    // portrait sensor mode is still turned a quarter turn. Three hardware rules
    // drive the geometry:
    //   * the SRM scale factor is quantised to 1/16 steps, so a plain float ratio
    //     leaves black bands;
    //   * the scaled output block must stay inside out.pic_w x out.pic_h, so the
    //     picture cannot simply be over-scaled and clipped;
    //   * the (rotated) block is placed at the output offset, so an exact-size
    //     block at (0,0) covers the whole output picture whatever the anchor is.
    // The crop is therefore derived from the quantised factor so that the scaled
    // output is exactly the panel size.
    static constexpr uint32_t CAMERA_PPA_SCALE_STEPS_PER_UNIT = 16;
    // Upscale limit searched when no factor gives an exact panel-size output.
    static constexpr uint32_t CAMERA_PPA_SCALE_SEARCH_STEPS = 16;

    struct CoverCropConfig {
        uint32_t crop_x;
        uint32_t crop_y;
        uint32_t crop_w;
        uint32_t crop_h;
        float scale_x;
        float scale_y;
        ppa_srm_rotation_angle_t rotation;
    };

    // Number of source pixels that the hardware scales to exactly `dst` pixels
    // at `steps`/16, or 0 when no such count exists. The PPA truncates the
    // scaled size, so the smallest count that reaches `dst` is the only one.
    static uint32_t exactKeep(uint32_t dst, uint32_t steps)
    {
        constexpr uint32_t STEPS = CAMERA_PPA_SCALE_STEPS_PER_UNIT;
        const uint64_t need = static_cast<uint64_t>(dst) * STEPS;
        const uint32_t keep = static_cast<uint32_t>((need + steps - 1) / steps);
        return (static_cast<uint64_t>(keep) * steps / STEPS == dst) ? keep : 0;
    }

    static CoverCropConfig computeCoverCrop(uint32_t src_w, uint32_t src_h, uint32_t dst_w, uint32_t dst_h)
    {
        constexpr uint32_t STEPS = CAMERA_PPA_SCALE_STEPS_PER_UNIT;
        CoverCropConfig crop = {
            .crop_x = 0,
            .crop_y = 0,
            .crop_w = src_w,
            .crop_h = src_h,
            .scale_x = 0.0f,
            .scale_y = 0.0f,
            .rotation = PPA_SRM_ROTATION_ANGLE_0,
        };

        if (src_w == 0 || src_h == 0 || dst_w == 0 || dst_h == 0) {
            return crop;
        }

        // Turn a portrait frame onto a landscape panel (and vice versa). After
        // the quarter turn the panel width is covered by the source height.
        const bool rotate = (src_h > src_w) != (dst_h > dst_w);
        crop.rotation = rotate ? PPA_SRM_ROTATION_ANGLE_270 : PPA_SRM_ROTATION_ANGLE_0;
        const uint32_t along_w = rotate ? src_h : src_w;
        const uint32_t along_h = rotate ? src_w : src_h;

        // Smallest 1/16 step that covers both panel directions.
        const uint64_t need_w = static_cast<uint64_t>(dst_w) * STEPS;
        const uint64_t need_h = static_cast<uint64_t>(dst_h) * STEPS;
        uint32_t steps = LV_MAX(static_cast<uint32_t>((need_w + along_w - 1) / along_w),
                                static_cast<uint32_t>((need_h + along_h - 1) / along_h));
        steps = steps ? steps : 1;

        // Above 1:1 one source pixel spans more than one output pixel, so a step
        // may have no exact crop; take the next step that has one.
        uint32_t keep_along_w = 0;
        uint32_t keep_along_h = 0;
        for (uint32_t s = steps; s <= steps + CAMERA_PPA_SCALE_SEARCH_STEPS; s++) {
            const uint32_t kw = exactKeep(dst_w, s);
            const uint32_t kh = exactKeep(dst_h, s);
            if (kw && kh && kw <= along_w && kh <= along_h) {
                steps = s;
                keep_along_w = kw;
                keep_along_h = kh;
                break;
            }
        }
        if (!keep_along_w) {
            // No exact fit: stay inside the panel and accept a 1px black edge.
            keep_along_w = LV_MIN(static_cast<uint32_t>(need_w / steps), along_w);
            keep_along_h = LV_MIN(static_cast<uint32_t>(need_h / steps), along_h);
        }
        keep_along_w = keep_along_w ? keep_along_w : 1;
        keep_along_h = keep_along_h ? keep_along_h : 1;

        const uint32_t keep_w = rotate ? keep_along_h : keep_along_w;
        const uint32_t keep_h = rotate ? keep_along_w : keep_along_h;
        crop.crop_x = (src_w - keep_w) / 2;
        crop.crop_y = (src_h - keep_h) / 2;
        crop.crop_w = keep_w;
        crop.crop_h = keep_h;
        crop.scale_x = static_cast<float>(steps) / static_cast<float>(STEPS);
        crop.scale_y = crop.scale_x;

        return crop;
    }

    Camera *Camera::_instance = nullptr;

    Camera *Camera::requestInstance(bool use_status_bar, bool use_navigation_bar)
    {
        if (_instance == nullptr)
        {
            _instance = new Camera(use_status_bar, use_navigation_bar);
        }
        return _instance;
    }

    Camera::Camera(bool use_status_bar, bool use_navigation_bar) : App("Camera", &img_app_camera, true, use_status_bar, use_navigation_bar)
    {
    }

    Camera::~Camera()
    {
        close();
    }

    bool Camera::run(void)
    {
        ESP_UTILS_LOGD("Run");

        _close_requested.store(false);
        _close_notification_posted.store(false);
        if (!createCloseRequestTimer()) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Create camera close request timer failed");
            return false;
        }

        bsp_extra_display_lock(-1);
        lv_obj_clean(lv_scr_act());
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, LV_PART_MAIN);
        _status_label = lv_label_create(lv_scr_act());
        lv_label_set_text(_status_label, "Camera");
        lv_obj_set_width(_status_label, BSP_LCD_H_RES);
        lv_obj_set_style_text_align(_status_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(_status_label, &lv_font_montserrat_30, LV_PART_MAIN);
        lv_obj_set_style_text_color(_status_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_align(_status_label, LV_ALIGN_CENTER, 0, 0);
        bsp_extra_display_unlock();

        if (!startPreview()) {
            bsp_extra_display_lock(-1);
            if (_status_label) {
                lv_label_set_text(_status_label, "camera error");
            }
            bsp_extra_display_unlock();
        }

        return true;
    }

    bool Camera::back(void)
    {
        ESP_UTILS_LOGD("Back");
        ESP_UTILS_CHECK_FALSE_RETURN(requestStopPreview(), false, "Stop camera preview failed");
        ESP_UTILS_CHECK_FALSE_RETURN(notifyCoreClosed(), false, "Notify core closed failed");
        return true;
    }

    bool Camera::close()
    {
        ESP_UTILS_LOGD("Close");
        if (!requestStopPreview()) {
            return false;
        }

        destroyCloseRequestTimer();

        bsp_extra_display_lock(-1);
        if (_status_label) {
            lv_obj_del(_status_label);
            _status_label = nullptr;
        }
        bsp_extra_display_unlock();
        return true;
    }

    bool Camera::init()
    {
        ESP_UTILS_LOGD("Init");
        return true;
    }

    bool Camera::deinit()
    {
        ESP_UTILS_LOGD("Deinit");
        return true;
    }

    bool Camera::pause()
    {
        ESP_UTILS_LOGD("Pause");
        return requestStopPreview();
    }

    bool Camera::resume()
    {
        ESP_UTILS_LOGD("Resume");
        return startPreview();
    }

    bool Camera::startPreview()
    {
        if (_preview_task_handle && _preview_exited.load()) {
            _preview_task_handle = nullptr;
        }
        if (_preview_task_handle) {
            return true;
        }

        _preview_exited.store(false);
        _preview_running = true;
        BaseType_t ret = xTaskCreatePinnedToCore(
            previewTaskEntry,
            "camera_preview",
            PREVIEW_TASK_STACK_SIZE,
            this,
            PREVIEW_TASK_PRIORITY,
            &_preview_task_handle,
            0);

        if (ret != pdPASS) {
            _preview_running = false;
            _preview_exited.store(true);
            _preview_task_handle = nullptr;
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Create camera preview task failed");
            return false;
        }

        return true;
    }

    bool Camera::createCloseRequestTimer()
    {
        if (_close_request_timer) {
            return true;
        }
        _close_request_timer = lv_timer_create(closeRequestTimerCallback, 20, this);
        return _close_request_timer != nullptr;
    }

    void Camera::destroyCloseRequestTimer()
    {
        if (_close_request_timer) {
            lv_timer_delete(_close_request_timer);
            _close_request_timer = nullptr;
        }
    }

    bool Camera::requestStopPreview()
    {
        if (_preview_task_handle && _preview_exited.load()) {
            _preview_task_handle = nullptr;
        }
        if (!_preview_task_handle) {
            stopDummyPreview();
            releaseCameraBuffers();
            return true;
        }

        if (xTaskGetCurrentTaskHandle() == _preview_task_handle) {
            _preview_running = false;
            return false;
        }

        _preview_running = false;
        TickType_t start = xTaskGetTickCount();
        while (!_preview_exited.load() &&
               xTaskGetTickCount() - start < pdMS_TO_TICKS(1500)) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        if (!_preview_exited.load()) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Timed out waiting for camera preview task");
            return false;
        }
        _preview_task_handle = nullptr;
        return true;
    }

    esp_err_t Camera::initVideoDriver()
    {
        if (_video_driver_initialized) {
            return ESP_OK;
        }

        esp_video_init_csi_config_t csi_config[] = {
            {
                .sccb_config = {
                    .init_sccb = false,
                    .i2c_handle = bsp_i2c_get_handle(),
                    .freq = CONFIG_BSP_I2C_CLK_SPEED_HZ,
                },
                .reset_pin = GPIO_NUM_NC,
                .pwdn_pin = GPIO_NUM_NC,
            },
        };

        esp_video_init_config_t cam_config = {
            .csi = csi_config,
        };

        esp_err_t ret = esp_video_init(&cam_config);
        if (ret == ESP_OK || ret == ESP_ERR_INVALID_STATE) {
            _video_driver_initialized = true;
            return ESP_OK;
        }

        ESP_LOGE(ESP_UTILS_LOG_TAG, "esp_video_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_err_t Camera::openVideoDevice()
    {
        if (_video_fd >= 0) {
            return ESP_OK;
        }

        _video_fd = open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, O_RDONLY);
        if (_video_fd < 0) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Open %s failed", ESP_VIDEO_MIPI_CSI_DEVICE_NAME);
            return ESP_FAIL;
        }

        struct v4l2_capability capability = {};
        if (ioctl(_video_fd, VIDIOC_QUERYCAP, &capability) != 0) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "VIDIOC_QUERYCAP failed");
            ::close(_video_fd);
            _video_fd = -1;
            return ESP_FAIL;
        }

        struct v4l2_format format = {};
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(_video_fd, VIDIOC_G_FMT, &format) != 0) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "VIDIOC_G_FMT failed");
            ::close(_video_fd);
            _video_fd = -1;
            return ESP_FAIL;
        }
        if (format.fmt.pix.width == 0 || format.fmt.pix.height == 0) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Camera returned an invalid capture size");
            ::close(_video_fd);
            _video_fd = -1;
            return ESP_ERR_INVALID_SIZE;
        }

        // The CSI node is initialized from the OV5647 sensor mode and rejects
        // width/height changes through VIDIOC_S_FMT. Keep that native size and
        // only request the LCD color format when the default differs.
        if (format.fmt.pix.pixelformat != CAMERA_VIDEO_FMT) {
            struct v4l2_format requested_format = format;
            requested_format.fmt.pix.pixelformat = CAMERA_VIDEO_FMT;
            if (ioctl(_video_fd, VIDIOC_S_FMT, &requested_format) != 0) {
                ESP_LOGE(
                    ESP_UTILS_LOG_TAG,
                    "VIDIOC_S_FMT color conversion failed for %" PRIu32 "x%" PRIu32,
                    format.fmt.pix.width,
                    format.fmt.pix.height
                );
                ::close(_video_fd);
                _video_fd = -1;
                return ESP_FAIL;
            }

            memset(&format, 0, sizeof(format));
            format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if (ioctl(_video_fd, VIDIOC_G_FMT, &format) != 0) {
                ESP_LOGE(ESP_UTILS_LOG_TAG, "VIDIOC_G_FMT after color conversion failed");
                ::close(_video_fd);
                _video_fd = -1;
                return ESP_FAIL;
            }
        }

        if (format.fmt.pix.pixelformat != CAMERA_VIDEO_FMT) {
            ESP_LOGE(
                ESP_UTILS_LOG_TAG,
                "Unsupported camera pixel format: 0x%08" PRIx32,
                format.fmt.pix.pixelformat
            );
            ::close(_video_fd);
            _video_fd = -1;
            return ESP_ERR_NOT_SUPPORTED;
        }

        _camera_width = format.fmt.pix.width;
        _camera_height = format.fmt.pix.height;
        _camera_buffer_size = format.fmt.pix.sizeimage;
        if (_camera_buffer_size == 0 && format.fmt.pix.bytesperline != 0) {
            _camera_buffer_size = format.fmt.pix.bytesperline * _camera_height;
        }
        if (_camera_buffer_size == 0) {
            _camera_buffer_size = _camera_width * _camera_height * CAMERA_BYTES_PER_PIXEL;
        }

        ESP_LOGI(
            ESP_UTILS_LOG_TAG,
            "Camera format: %" PRIu32 "x%" PRIu32 ", buffer: %u bytes",
            _camera_width,
            _camera_height,
            static_cast<unsigned>(_camera_buffer_size)
        );

        return ESP_OK;
    }

    esp_err_t Camera::setupCameraBuffers()
    {
#ifdef CONFIG_CACHE_L2_CACHE_LINE_SIZE
        _data_cache_line_size = CONFIG_CACHE_L2_CACHE_LINE_SIZE;
#else
        _data_cache_line_size = 64;
#endif

        struct v4l2_requestbuffers req = {};
        req.count = CAMERA_BUFFER_COUNT;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_USERPTR;

        if (ioctl(_video_fd, VIDIOC_REQBUFS, &req) != 0) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "VIDIOC_REQBUFS failed");
            return ESP_FAIL;
        }

        for (int i = 0; i < CAMERA_BUFFER_COUNT; i++) {
            struct v4l2_buffer buf = {};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;
            buf.index = i;

            if (ioctl(_video_fd, VIDIOC_QUERYBUF, &buf) != 0) {
                ESP_LOGE(ESP_UTILS_LOG_TAG, "VIDIOC_QUERYBUF %d failed", i);
                return ESP_FAIL;
            }

            size_t alloc_size = buf.length ? buf.length : _camera_buffer_size;
            _camera_buffer_lengths[i] = alloc_size;
            _camera_buffers[i] = heap_caps_aligned_calloc(_data_cache_line_size, 1, alloc_size, MALLOC_CAP_SPIRAM);
            if (!_camera_buffers[i]) {
                ESP_LOGE(ESP_UTILS_LOG_TAG, "Camera buffer %d allocation failed", i);
                return ESP_ERR_NO_MEM;
            }

            buf.m.userptr = (unsigned long)_camera_buffers[i];
            buf.length = alloc_size;

            if (ioctl(_video_fd, VIDIOC_QBUF, &buf) != 0) {
                ESP_LOGE(ESP_UTILS_LOG_TAG, "VIDIOC_QBUF %d failed", i);
                return ESP_FAIL;
            }
        }

        return ESP_OK;
    }

    esp_err_t Camera::startDummyPreview()
    {
        _display = lv_display_get_default();
        ESP_RETURN_ON_FALSE(_display != nullptr, ESP_ERR_INVALID_STATE, ESP_UTILS_LOG_TAG, "LVGL display is not ready");

        _display_panel = bsp_display_get_panel_handle();
        ESP_RETURN_ON_FALSE(_display_panel != nullptr, ESP_ERR_INVALID_STATE, ESP_UTILS_LOG_TAG, "LCD panel is not ready");

        esp_err_t ret = ESP_OK;
#if CONFIG_BSP_LCD_DPI_BUFFER_NUMS >= 3
        ret = esp_lcd_dpi_panel_get_frame_buffer(
                  _display_panel,
                  CONFIG_BSP_LCD_DPI_BUFFER_NUMS,
                  &_lcd_buffers[0],
                  &_lcd_buffers[1],
                  &_lcd_buffers[2]
              );
#elif CONFIG_BSP_LCD_DPI_BUFFER_NUMS == 2
        ret = esp_lcd_dpi_panel_get_frame_buffer(
                  _display_panel,
                  CONFIG_BSP_LCD_DPI_BUFFER_NUMS,
                  &_lcd_buffers[0],
                  &_lcd_buffers[1]
              );
#else
#error "Camera preview needs at least two LCD frame buffers"
#endif
        ESP_RETURN_ON_ERROR(ret, ESP_UTILS_LOG_TAG, "Get LCD frame buffers failed");
        _current_lcd_buffer_index = 0;

        if (_ppa_srm_handle == nullptr) {
            ppa_client_config_t ppa_srm_config = {
                .oper_type = PPA_OPERATION_SRM,
            };
            ESP_RETURN_ON_ERROR(
                ppa_register_client(&ppa_srm_config, &_ppa_srm_handle),
                ESP_UTILS_LOG_TAG,
                "Register PPA SRM failed"
            );
        }

        ret = esp_lv_adapter_set_dummy_draw(_display, true);
        if (ret != ESP_OK) {
            stopDummyPreview();
            return ret;
        }
        _dummy_enabled = true;

        ret = esp_lv_adapter_pause(-1);
        if (ret != ESP_OK) {
            stopDummyPreview();
            return ret;
        }
        _lvgl_paused = true;

        ret = bsp_touch_get_shared(&_touch_handle);
        if (ret != ESP_OK) {
            stopDummyPreview();
            return ret;
        }

        // Clear and flush every framebuffer once so the first published frame
        // never shows stale panel content.
        const size_t lcd_buffer_size =
            static_cast<size_t>(BSP_LCD_H_RES) * BSP_LCD_V_RES * CAMERA_BYTES_PER_PIXEL;
        for (int i = 0; i < CONFIG_BSP_LCD_DPI_BUFFER_NUMS; i++) {
            memset(_lcd_buffers[i], 0, lcd_buffer_size);
            ret = esp_cache_msync(_lcd_buffers[i], lcd_buffer_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
            if (ret != ESP_OK) {
                stopDummyPreview();
                return ret;
            }
        }

        const CoverCropConfig crop = computeCoverCrop(
            _camera_width,
            _camera_height,
            BSP_LCD_H_RES,
            BSP_LCD_V_RES
        );

        ESP_LOGI(
            ESP_UTILS_LOG_TAG,
            "Camera preview cover-fills %" PRIu32 "x%" PRIu32 " rotated %ddeg: block %" PRIu32 ",%" PRIu32
            " %" PRIu32 "x%" PRIu32 " scale %.4f on %dx%d, %d frame buffers",
            _camera_width,
            _camera_height,
            crop.rotation == PPA_SRM_ROTATION_ANGLE_270 ? 270 : 0,
            crop.crop_x,
            crop.crop_y,
            crop.crop_w,
            crop.crop_h,
            crop.scale_x,
            BSP_LCD_H_RES,
            BSP_LCD_V_RES,
            CONFIG_BSP_LCD_DPI_BUFFER_NUMS
        );
        return ESP_OK;
    }

    void Camera::stopDummyPreview()
    {
        // Stop direct reads before the LVGL adapter resumes using the same
        // BSP-owned GT911 handle.
        _touch_handle = nullptr;
        _touch_active = false;

        if (_dummy_enabled && _display) {
            esp_lv_adapter_set_dummy_draw(_display, false);
            _dummy_enabled = false;
        }

        if (_lvgl_paused) {
            esp_lv_adapter_resume();
            _lvgl_paused = false;
        }

        memset(_lcd_buffers, 0, sizeof(_lcd_buffers));
        _display_panel = nullptr;
        _display = nullptr;
        _current_lcd_buffer_index = 0;
    }

    void Camera::releaseCameraBuffers()
    {
        for (int i = 0; i < CAMERA_BUFFER_COUNT; i++) {
            if (_camera_buffers[i]) {
                heap_caps_free(_camera_buffers[i]);
                _camera_buffers[i] = nullptr;
            }
            _camera_buffer_lengths[i] = 0;
        }

        if (_video_fd >= 0) {
            ::close(_video_fd);
            _video_fd = -1;
        }
    }

    bool Camera::shouldExitBySwipe()
    {
        if (!_touch_handle) {
            return false;
        }

        esp_lcd_touch_read_data(_touch_handle);

        esp_lcd_touch_point_data_t points[1] = {};
        uint8_t point_num = 0;
        esp_err_t ret = esp_lcd_touch_get_data(_touch_handle, points, &point_num, 1);

        if (ret != ESP_OK || point_num == 0) {
            _touch_active = false;
            return false;
        }

        if (!_touch_active) {
            _touch_active = true;
            _touch_start_x = points[0].x;
            _touch_start_y = points[0].y;
            return false;
        }

        int dx = abs((int)points[0].x - (int)_touch_start_x);
        int dy = abs((int)points[0].y - (int)_touch_start_y);
        return dx > SWIPE_EXIT_THRESHOLD || dy > SWIPE_EXIT_THRESHOLD;
    }

    esp_err_t Camera::handleFrame()
    {
        const int lcd_buffer_index =
            (_current_lcd_buffer_index + 1) % CONFIG_BSP_LCD_DPI_BUFFER_NUMS;
        ESP_RETURN_ON_FALSE(
            _lcd_buffers[lcd_buffer_index] != nullptr,
            ESP_ERR_INVALID_STATE,
            ESP_UTILS_LOG_TAG,
            "LCD frame buffer %d is not ready",
            lcd_buffer_index
        );

        struct v4l2_buffer v4l2_buf = {};
        v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.memory = V4L2_MEMORY_USERPTR;

        if (ioctl(_video_fd, VIDIOC_DQBUF, &v4l2_buf) != 0) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "VIDIOC_DQBUF failed");
            return ESP_FAIL;
        }
        if (v4l2_buf.index >= CAMERA_BUFFER_COUNT) {
            ESP_LOGE(
                ESP_UTILS_LOG_TAG,
                "Camera returned invalid buffer index: %u",
                static_cast<unsigned>(v4l2_buf.index)
            );
            return ESP_ERR_INVALID_STATE;
        }

        const uint32_t display_w = BSP_LCD_H_RES;
        const uint32_t display_h = BSP_LCD_V_RES;
        const CoverCropConfig crop = computeCoverCrop(_camera_width, _camera_height, display_w, display_h);
        ESP_RETURN_ON_FALSE((crop.scale_x > 0.0f) && (crop.scale_y > 0.0f),
                            ESP_ERR_INVALID_SIZE, ESP_UTILS_LOG_TAG, "Invalid camera fit");

        ppa_srm_oper_config_t srm_config = {};
        srm_config.in.buffer = _camera_buffers[v4l2_buf.index];
        srm_config.in.pic_w = _camera_width;
        srm_config.in.pic_h = _camera_height;
        srm_config.in.block_w = crop.crop_w;
        srm_config.in.block_h = crop.crop_h;
        srm_config.in.block_offset_x = crop.crop_x;
        srm_config.in.block_offset_y = crop.crop_y;
        srm_config.in.srm_cm = CAMERA_PPA_COLOR_MODE;

        srm_config.out.buffer = _lcd_buffers[lcd_buffer_index];
        srm_config.out.buffer_size = ALIGN_UP(
            display_w * display_h * CAMERA_BYTES_PER_PIXEL,
            _data_cache_line_size
        );
        srm_config.out.pic_w = display_w;
        srm_config.out.pic_h = display_h;
        srm_config.out.block_offset_x = 0;
        srm_config.out.block_offset_y = 0;
        srm_config.out.srm_cm = CAMERA_PPA_COLOR_MODE;

        srm_config.rotation_angle = crop.rotation;
        srm_config.scale_x = crop.scale_x;
        srm_config.scale_y = crop.scale_y;
        // Match the LCD-X/4B/5/Platform camera orientation: the previous 180
        // degree flip (mirror_x = mirror_y = 1) did not correspond to the panel
        // rotation, which the touch correction left unchanged.
        srm_config.mirror_x = 0;
        srm_config.mirror_y = 0;
        srm_config.rgb_swap = 0;
        srm_config.byte_swap = 0;
        srm_config.mode = PPA_TRANS_MODE_BLOCKING;

        esp_err_t ret = ppa_do_scale_rotate_mirror(_ppa_srm_handle, &srm_config);
        if (ret == ESP_OK && _dummy_enabled) {
            ret = esp_lv_adapter_dummy_draw_blit(
                      _display,
                      0,
                      0,
                      display_w,
                      display_h,
                      _lcd_buffers[lcd_buffer_index],
                      true
                  );
            if (ret == ESP_OK) {
                _current_lcd_buffer_index = lcd_buffer_index;
            }
        }

        v4l2_buf.m.userptr = (unsigned long)_camera_buffers[v4l2_buf.index];
        v4l2_buf.length = _camera_buffer_lengths[v4l2_buf.index];
        if (ioctl(_video_fd, VIDIOC_QBUF, &v4l2_buf) != 0) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "VIDIOC_QBUF failed");
            return ESP_FAIL;
        }

        return ret;
    }

    void Camera::previewTask()
    {
        bool request_close = false;
        esp_err_t ret = initVideoDriver();
        if (ret == ESP_OK) {
            ret = openVideoDevice();
        }
        if (ret == ESP_OK) {
            ret = setupCameraBuffers();
        }
        if (ret == ESP_OK) {
            ret = startDummyPreview();
        }

        if (ret == ESP_OK) {
            int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if (ioctl(_video_fd, VIDIOC_STREAMON, &type) != 0) {
                ESP_LOGE(ESP_UTILS_LOG_TAG, "VIDIOC_STREAMON failed");
                ret = ESP_FAIL;
            }
        }

        while (_preview_running && ret == ESP_OK) {
            ret = handleFrame();
            if (shouldExitBySwipe()) {
                request_close = true;
                _preview_running = false;
            }
        }

        if (_video_fd >= 0) {
            int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            ioctl(_video_fd, VIDIOC_STREAMOFF, &type);
        }

        stopDummyPreview();
        releaseCameraBuffers();

        if (request_close) {
            // This worker is pinned to core 0. The GUI-owned timer observes
            // this request from LVGL's context before it calls notifyCoreClosed().
            _close_requested.store(true);
        }

        _preview_running = false;
        _preview_exited.store(true);

        vTaskDelete(NULL);
    }

    void Camera::previewTaskEntry(void *arg)
    {
        static_cast<Camera *>(arg)->previewTask();
    }

    void Camera::closeRequestTimerCallback(lv_timer_t *timer)
    {
        auto *self = static_cast<Camera *>(lv_timer_get_user_data(timer));
        if (!self || !self->_close_requested.load()) {
            return;
        }

        bool expected = false;
        if (!self->_close_notification_posted.compare_exchange_strong(expected, true)) {
            return;
        }
        if (!self->notifyCoreClosed()) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Notify camera close failed");
            self->_close_notification_posted.store(false);
        }
    }

} // namespace esp_brookesia::apps
