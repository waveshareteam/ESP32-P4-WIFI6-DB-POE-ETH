/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "VideoPlayer.hpp"
#include "brookesia/service_helper/system/storage.hpp"
#include <algorithm>
#include "lvgl.h"
#include "esp_brookesia.hpp"

#include "dev_fs_fat.h"
#include "dev_lcd_touch.h"
#include "esp_board_manager.h"
#include "esp_board_manager_defs.h"
#include "esp_lib_utils.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include <limits.h>
#include <stdio.h>
#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:VideoPlayer"

LV_IMG_DECLARE(img_app_vedioplayer);

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

static constexpr uint32_t VIDEO_DEFAULT_SAMPLE_RATE = 48000;
static constexpr uint8_t VIDEO_DEFAULT_BIT_WIDTH = 16;

namespace esp_brookesia::apps
{

    static int read_saved_media_volume()
    {
        // Match the namespace, key, default and range used by Settings/SoundPage.
        constexpr int default_volume = 70;
        auto result = service::helper::Storage::get_key_value<int32_t>("storage", "media_volume", 1000);
        return result ? std::clamp(static_cast<int>(result.value()), 0, 95) : default_volume;
    }

    VideoPlayer *VideoPlayer::_instance = nullptr;

    VideoPlayer *VideoPlayer::requestInstance(bool use_status_bar, bool use_navigation_bar)
    {
        if (_instance == nullptr)
        {
            _instance = new VideoPlayer(use_status_bar, use_navigation_bar);
        }
        return _instance;
    }

    VideoPlayer::VideoPlayer(bool use_status_bar, bool use_navigation_bar) : App("VideoPlayer", &img_app_vedioplayer, true, use_status_bar, use_navigation_bar), sd_mounted(false)
    {
        display = nullptr;
        ppa_srm_handle = nullptr;
        touch_handle = nullptr;
        jpeg_handle = nullptr;
        jpeg_output_buffer = nullptr;
        jpeg_output_buffer_size = 0;
        avi_player_handle = nullptr;
        play_task_handle = nullptr;
        close_wait_task = nullptr;
        is_playing = false;
        is_paused = false;
        loop_playback = false;
        dummy_enabled = false;
        lvgl_paused = false;
        touch_active = false;
        avi_file_list = nullptr;
        avi_file_count = 0;
        last_video_width = 0;
        last_video_height = 0;
    }

    VideoPlayer::~VideoPlayer()
    {
        close();
    }

    bool VideoPlayer::run(void)
    {
        ESP_UTILS_LOGD("Run");
        is_paused = false;

        display = lv_display_get_default();
        if (!display) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "LVGL display is not ready");
            return false;
        }
        display_width = lv_display_get_horizontal_resolution(display);
        display_height = lv_display_get_vertical_resolution(display);
        if (display_width <= 0 || display_height <= 0) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Invalid display size: %dx%d", display_width, display_height);
            return false;
        }

        const esp_err_t mount_ret = mountSdCard();
        sd_mounted = mount_ret == ESP_OK;
        if (!sd_mounted) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Mount SD card through Board Manager failed: %s", esp_err_to_name(mount_ret));
        }

        if (!lockDisplay()) {
            return false;
        }
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, LV_PART_MAIN);
        status_label = lv_label_create(lv_scr_act());
        lv_label_set_text(status_label, sd_mounted ? "loading files..." : "sd error");
        lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(status_label, display_width);
        lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_font(status_label, &lv_font_montserrat_20, 0);
        unlockDisplay();

        if (!sd_mounted)
        {
            return true;
        }

        if (getAviFileList(avi_dir_path) != ESP_OK || avi_file_count == 0)
        {
            if (!lockDisplay()) {
                return false;
            }
            lv_label_set_text(status_label, "avi error");
            unlockDisplay();
            return true;
        }

        if (!lockDisplay()) {
            return false;
        }
        lv_obj_del(status_label);
        status_label = nullptr;
        unlockDisplay();

        return startPlaybackTask();
    }

    bool VideoPlayer::back(void)
    {
        ESP_UTILS_LOGD("Back");
        // If the app needs to exit, call notifyCoreClosed() to notify the core to close the app
        ESP_UTILS_CHECK_FALSE_RETURN(notifyCoreClosed(), false, "Notify core closed failed");
        return true;
    }

    bool VideoPlayer::close()
    {
        ESP_UTILS_LOGD("Close");
        if (!stopPlaybackTask(pdMS_TO_TICKS(2000))) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Playback task did not stop before close");
            return false;
        }

        deinitJpegDecoder();
        releaseJpegOutputBuffer();
        deinitDisplayBypass();

        if (lockDisplay()) {
            if (status_label) {
                lv_obj_del(status_label);
                status_label = nullptr;
            }
            unlockDisplay();
        }

        if (avi_file_list)
        {
            for (int i = 0; i < avi_file_count; i++)
            {
                free(avi_file_list[i]);
            }
            free(avi_file_list);
            avi_file_list = nullptr;
            avi_file_count = 0;
        }

        const esp_err_t sd_ret = unmountSdCard();
        if (sd_ret != ESP_OK) {
            ESP_LOGW(ESP_UTILS_LOG_TAG, "Deinit SD card through Board Manager failed: %s", esp_err_to_name(sd_ret));
        }

        return true;
    }

    bool VideoPlayer::init()
    {
        ESP_UTILS_LOGD("Init");
        // Mount storage only when the user opens VideoPlayer.
        return true;
    }

    bool VideoPlayer::deinit()
    {
        ESP_UTILS_LOGD("Deinit");
        const esp_err_t ret = unmountSdCard();
        if (ret != ESP_OK) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Deinit SD card device failed: %s", esp_err_to_name(ret));
            return false;
        }
        return true;
    }

    bool VideoPlayer::pause()
    {
        ESP_UTILS_LOGD("Pause");
        return stopPlaybackTask(pdMS_TO_TICKS(2000));
    }

    bool VideoPlayer::resume()
    {
        ESP_UTILS_LOGD("Resume");
        if (!sd_mounted || !avi_file_list || avi_file_count == 0) {
            return true;
        }
        return startPlaybackTask();
    }

    bool VideoPlayer::startPlaybackTask()
    {
        if (play_task_handle) {
            return true;
        }
        if (!avi_file_list || avi_file_count == 0) {
            return false;
        }

        loop_playback = true;
        is_paused = false;

        // AVI parsing, JPEG decoding, PPA blitting, and PCM output share this task.
        BaseType_t ret = xTaskCreatePinnedToCore(
            playTask,
            "avi_play_task",
            32768,
            this,
            7,
            &play_task_handle,
            0
        );
        if (ret != pdPASS) {
            loop_playback = false;
            play_task_handle = nullptr;
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Create playback task failed");
            return false;
        }

        return true;
    }

    bool VideoPlayer::stopPlaybackTask(TickType_t timeout)
    {
        loop_playback = false;
        is_paused = false;

        if (!play_task_handle) {
            return true;
        }
        if (xTaskGetCurrentTaskHandle() == play_task_handle) {
            return false;
        }

        close_wait_task = xTaskGetCurrentTaskHandle();
        if (!play_task_handle) {
            close_wait_task = nullptr;
            return true;
        }

        if (avi_player_handle) {
            avi_player_play_stop(avi_player_handle);
        }

        bool stopped = ulTaskNotifyTake(pdTRUE, timeout) != 0;
        close_wait_task = nullptr;

        if (!stopped || play_task_handle) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Timed out waiting for playback task");
            return false;
        }

        return true;
    }

    esp_err_t VideoPlayer::mountSdCard()
    {
        if (sd_device_initialized) {
            return sd_mounted ? ESP_OK : ESP_ERR_INVALID_STATE;
        }
        if (!esp_board_manager_check_name(ESP_BOARD_DEVICE_NAME_FS_SDCARD)) {
            return ESP_ERR_NOT_FOUND;
        }

        esp_err_t ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_FS_SDCARD);
        if (ret != ESP_OK) {
            return ret;
        }
        sd_device_initialized = true;

        dev_fs_fat_handle_t *fs_handle = nullptr;
        ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_FS_SDCARD, (void **)&fs_handle);
        if (ret != ESP_OK || fs_handle == nullptr || fs_handle->card == nullptr ||
                fs_handle->mount_point == nullptr || fs_handle->mount_point[0] == '\0') {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Get mounted SD card handle failed");
            (void)unmountSdCard();
            return (ret == ESP_OK) ? ESP_ERR_INVALID_STATE : ret;
        }

        const size_t mount_len = strlen(fs_handle->mount_point);
        const char *separator = fs_handle->mount_point[mount_len - 1] == '/' ? "" : "/";
        const int path_len = snprintf(avi_dir_path, sizeof(avi_dir_path), "%s%savi", fs_handle->mount_point, separator);
        if (path_len < 0 || static_cast<size_t>(path_len) >= sizeof(avi_dir_path)) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "AVI directory path is too long");
            (void)unmountSdCard();
            return ESP_ERR_INVALID_SIZE;
        }

        sd_mounted = true;
        ESP_UTILS_LOGD("SD card mounted at %s", fs_handle->mount_point);
        return ESP_OK;
    }

    esp_err_t VideoPlayer::unmountSdCard()
    {
        if (!sd_device_initialized) {
            sd_mounted = false;
            avi_dir_path[0] = '\0';
            return ESP_OK;
        }

        const esp_err_t ret = esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_FS_SDCARD);
        if (ret == ESP_OK) {
            sd_device_initialized = false;
            sd_mounted = false;
            avi_dir_path[0] = '\0';
        }
        return ret;
    }

    esp_err_t VideoPlayer::initAudioDevice()
    {
        if (audio_device_initialized && audio_handles != nullptr && audio_handles->codec_dev != nullptr) {
            audio_ready = true;
            return ESP_OK;
        }
        if (!esp_board_manager_check_name(ESP_BOARD_DEVICE_NAME_AUDIO_DAC)) {
            return ESP_ERR_NOT_FOUND;
        }

        esp_err_t ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_AUDIO_DAC);
        if (ret != ESP_OK) {
            return ret;
        }
        audio_device_initialized = true;

        ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_AUDIO_DAC, (void **)&audio_handles);
        if (ret != ESP_OK || audio_handles == nullptr || audio_handles->codec_dev == nullptr) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Get audio DAC handle failed");
            deinitAudioDevice();
            return (ret == ESP_OK) ? ESP_ERR_INVALID_STATE : ret;
        }

        audio_ready = true;
        return ESP_OK;
    }

    void VideoPlayer::deinitAudioDevice()
    {
        audio_ready = false;
        if (audio_codec_open && audio_handles != nullptr && audio_handles->codec_dev != nullptr) {
            const int ret = esp_codec_dev_close(audio_handles->codec_dev);
            if (ret != ESP_CODEC_DEV_OK) {
                ESP_LOGW(ESP_UTILS_LOG_TAG, "Close audio codec failed: %d", ret);
            }
            audio_codec_open = false;
        }
        audio_format = {};

        if (audio_device_initialized) {
            const esp_err_t ret = esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_AUDIO_DAC);
            if (ret != ESP_OK) {
                ESP_LOGW(ESP_UTILS_LOG_TAG, "Deinit audio DAC device failed: %s", esp_err_to_name(ret));
            } else {
                audio_device_initialized = false;
                audio_handles = nullptr;
            }
        }
    }

    esp_err_t VideoPlayer::configureAudio(uint32_t rate, uint32_t bits, uint32_t channels)
    {
        if (!audio_ready || audio_handles == nullptr || audio_handles->codec_dev == nullptr) {
            return ESP_ERR_INVALID_STATE;
        }

        esp_codec_dev_sample_info_t sample_info = {
            .bits_per_sample = static_cast<uint8_t>(bits ? bits : VIDEO_DEFAULT_BIT_WIDTH),
            .channel = static_cast<uint8_t>(channels <= 1 ? 1 : 2),
            .sample_rate = rate ? rate : VIDEO_DEFAULT_SAMPLE_RATE,
        };
        if (audio_codec_open && audio_format.sample_rate == sample_info.sample_rate &&
                audio_format.channel == sample_info.channel &&
                audio_format.bits_per_sample == sample_info.bits_per_sample) {
            return ESP_OK;
        }

        // Another app may still own the shared DAC while paused. Do not rely on
        // esp_codec_dev_open() here: it returns success for an already-open device
        // without applying the requested sample format.
        if (!audio_codec_open && audio_handles->data_if != nullptr &&
                audio_handles->data_if->is_open != nullptr &&
                audio_handles->data_if->is_open(audio_handles->data_if)) {
            audio_ready = false;
            return ESP_ERR_INVALID_STATE;
        }

        if (audio_codec_open) {
            const int close_ret = esp_codec_dev_close(audio_handles->codec_dev);
            if (close_ret != ESP_CODEC_DEV_OK) {
                audio_ready = false;
                return static_cast<esp_err_t>(close_ret);
            }
            audio_codec_open = false;
        }

        const int open_ret = esp_codec_dev_open(audio_handles->codec_dev, &sample_info);
        if (open_ret != ESP_CODEC_DEV_OK) {
            audio_ready = false;
            return static_cast<esp_err_t>(open_ret);
        }
        audio_codec_open = true;
        audio_format = sample_info;

        const int volume_ret = esp_codec_dev_set_out_vol(audio_handles->codec_dev, read_saved_media_volume());
        if (volume_ret != ESP_CODEC_DEV_OK) {
            ESP_LOGW(ESP_UTILS_LOG_TAG, "Set video volume failed: %d", volume_ret);
        }
        return ESP_OK;
    }

    bool VideoPlayer::lockDisplay()
    {
        const esp_err_t ret = esp_lv_adapter_lock(-1);
        if (ret != ESP_OK) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Lock display failed: %s", esp_err_to_name(ret));
            return false;
        }
        return true;
    }

    void VideoPlayer::unlockDisplay()
    {
        esp_lv_adapter_unlock();
    }

    esp_err_t VideoPlayer::getAviFileList(const char *dir_path)
    {
        DIR *dir = opendir(dir_path);
        if (!dir)
            return ESP_FAIL;

        struct dirent *entry;
        int count = 0;

        while ((entry = readdir(dir)) != nullptr)
        {
            if (entry->d_type == DT_REG)
            {
                char *ext = strrchr(entry->d_name, '.');
                if (ext && strcasecmp(ext, ".avi") == 0)
                {
                    count++;
                }
            }
        }

        if (count == 0)
        {
            closedir(dir);
            return ESP_FAIL;
        }

        avi_file_list = (char **)malloc(sizeof(char *) * count);
        if (!avi_file_list)
        {
            closedir(dir);
            return ESP_ERR_NO_MEM;
        }

        avi_file_count = count;
        int loaded_count = 0;
        rewinddir(dir);

        while ((entry = readdir(dir)) != nullptr)
        {
            if (entry->d_type == DT_REG)
            {
                char *ext = strrchr(entry->d_name, '.');
                if (ext && strcasecmp(ext, ".avi") == 0)
                {
                    size_t len = strlen(dir_path) + strlen(entry->d_name) + 2;
                    char *full_path = (char *)malloc(len);
                    if (!full_path) {
                        for (int i = 0; i < loaded_count; i++) {
                            free(avi_file_list[i]);
                        }
                        free(avi_file_list);
                        avi_file_list = nullptr;
                        avi_file_count = 0;
                        closedir(dir);
                        return ESP_ERR_NO_MEM;
                    }
                    snprintf(full_path, len, "%s/%s", dir_path, entry->d_name);
                    avi_file_list[loaded_count++] = full_path;
                }
            }
        }

        closedir(dir);
        return ESP_OK;
    }

    esp_err_t VideoPlayer::initDisplayBypass()
    {
#ifdef CONFIG_CACHE_L2_CACHE_LINE_SIZE
        data_cache_line_size = CONFIG_CACHE_L2_CACHE_LINE_SIZE;
#else
        data_cache_line_size = 64;
#endif

        display = lv_display_get_default();
        ESP_RETURN_ON_FALSE(display != nullptr, ESP_ERR_INVALID_STATE, ESP_UTILS_LOG_TAG, "LVGL display is not ready");
        display_width = lv_display_get_horizontal_resolution(display);
        display_height = lv_display_get_vertical_resolution(display);
        ESP_RETURN_ON_FALSE(display_width > 0 && display_height > 0, ESP_ERR_INVALID_SIZE, ESP_UTILS_LOG_TAG,
                            "Invalid display size: %dx%d", display_width, display_height);

        display_rgb888 = false;
        display_rgb565_swapped = false;
        switch (lv_display_get_color_format(display)) {
        case LV_COLOR_FORMAT_RGB565:
            jpeg_output_format = JPEG_PIXEL_FORMAT_RGB565_LE;
            video_bytes_per_pixel = 2;
            break;
        case LV_COLOR_FORMAT_RGB565_SWAPPED:
            jpeg_output_format = JPEG_PIXEL_FORMAT_RGB565_LE;
            video_bytes_per_pixel = 2;
            display_rgb565_swapped = true;
            break;
        case LV_COLOR_FORMAT_RGB888:
            jpeg_output_format = JPEG_PIXEL_FORMAT_RGB888;
            video_bytes_per_pixel = 3;
            display_rgb888 = true;
            break;
        default:
            return ESP_ERR_NOT_SUPPORTED;
        }

        if (ppa_srm_handle == nullptr) {
            ppa_client_config_t ppa_srm_config = {
                .oper_type = PPA_OPERATION_SRM,
            };
            ESP_RETURN_ON_ERROR(ppa_register_client(&ppa_srm_config, &ppa_srm_handle), ESP_UTILS_LOG_TAG, "Register PPA SRM failed");
        }

        touch_handle = nullptr;
        if (esp_board_manager_check_name(ESP_BOARD_DEVICE_NAME_LCD_TOUCH)) {
            dev_lcd_touch_handles_t *touch_handles = nullptr;
            if (esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_LCD_TOUCH, (void **)&touch_handles) == ESP_OK &&
                    touch_handles != nullptr) {
                touch_handle = touch_handles->touch_handle;
            }
        }

        ESP_RETURN_ON_ERROR(esp_lv_adapter_set_dummy_draw(display, true), ESP_UTILS_LOG_TAG, "Enable dummy draw failed");
        dummy_enabled = true;

        ESP_RETURN_ON_ERROR(esp_lv_adapter_pause(-1), ESP_UTILS_LOG_TAG, "Pause LVGL failed");
        lvgl_paused = true;

        return ESP_OK;
    }

    void VideoPlayer::deinitDisplayBypass()
    {
        if (lvgl_paused) {
            esp_lv_adapter_resume();
            lvgl_paused = false;
        }

        if (dummy_enabled && display) {
            esp_lv_adapter_set_dummy_draw(display, false);
            dummy_enabled = false;
        }

        // The touch device is initialized and owned by the application-level Board Manager setup.
        touch_handle = nullptr;

        touch_active = false;
        last_video_width = 0;
        last_video_height = 0;
    }

    esp_err_t VideoPlayer::initJpegDecoder()
    {
        if (jpeg_handle != nullptr)
            return ESP_OK;

        jpeg_dec_config_t config = DEFAULT_JPEG_DEC_CONFIG();
        config.output_type = jpeg_output_format;

        jpeg_error_t err = jpeg_dec_open(&config, &jpeg_handle);
        return (err == JPEG_ERR_OK) ? ESP_OK : ESP_FAIL;
    }

    void VideoPlayer::deinitJpegDecoder()
    {
        if (jpeg_handle)
        {
            jpeg_dec_close(jpeg_handle);
            jpeg_handle = nullptr;
        }
    }

    esp_err_t VideoPlayer::ensureJpegOutputBuffer(int out_len)
    {
        if (out_len <= 0) {
            return ESP_ERR_INVALID_SIZE;
        }

        if (jpeg_output_buffer && jpeg_output_buffer_size >= out_len) {
            return ESP_OK;
        }

        releaseJpegOutputBuffer();
        size_t alloc_size = ALIGN_UP(static_cast<size_t>(out_len), 16);
        jpeg_output_buffer = static_cast<uint8_t *>(
            heap_caps_aligned_calloc(16, 1, alloc_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (!jpeg_output_buffer) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "JPEG output buffer allocation failed: %u bytes", static_cast<unsigned>(alloc_size));
            return ESP_ERR_NO_MEM;
        }

        jpeg_output_buffer_size = static_cast<int>(alloc_size);
        return ESP_OK;
    }

    void VideoPlayer::releaseJpegOutputBuffer()
    {
        if (jpeg_output_buffer) {
            heap_caps_free(jpeg_output_buffer);
            jpeg_output_buffer = nullptr;
            jpeg_output_buffer_size = 0;
        }
    }

    void VideoPlayer::videoCallback(frame_data_t *data, void *arg)
    {
        if (!data || !data->data || data->data_bytes == 0)
            return;

        VideoPlayer *self = static_cast<VideoPlayer *>(arg);

        if (self->initJpegDecoder() != ESP_OK || !self->dummy_enabled || !self->ppa_srm_handle)
            return;

        // Decode each MJPEG frame into a PSRAM buffer, then use PPA SRM to scale it
        // into the next LCD frame buffer without involving LVGL widgets.
        jpeg_dec_io_t io = {
            .inbuf = data->data,
            .inbuf_len = static_cast<int>(data->data_bytes),
            .outbuf = nullptr,
        };

        jpeg_dec_header_info_t info;
        if (jpeg_dec_parse_header(self->jpeg_handle, &io, &info) != JPEG_ERR_OK) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "JPEG header parse failed");
            return;
        }

        if (info.width == 0 || info.height == 0) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Invalid JPEG frame size: %dx%d", info.width, info.height);
            return;
        }

        int out_len = 0;
        if (jpeg_dec_get_outbuf_len(self->jpeg_handle, &out_len) != JPEG_ERR_OK ||
            self->ensureJpegOutputBuffer(out_len) != ESP_OK) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Prepare JPEG output buffer failed");
            return;
        }

        io.outbuf = self->jpeg_output_buffer;
        if (jpeg_dec_process(self->jpeg_handle, &io) != JPEG_ERR_OK) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "JPEG decode process failed");
            return;
        }

        uint32_t crop_x = 0;
        uint32_t crop_y = 0;
        uint32_t crop_w = info.width;
        uint32_t crop_h = info.height;

        if (static_cast<uint64_t>(info.width) * self->display_height >
                static_cast<uint64_t>(info.height) * self->display_width) {
            crop_w = static_cast<uint32_t>(
                (static_cast<uint64_t>(info.height) * self->display_width) / self->display_height
            );
            crop_x = (info.width - crop_w) / 2;
        } else {
            crop_h = static_cast<uint32_t>(
                (static_cast<uint64_t>(info.width) * self->display_height) / self->display_width
            );
            crop_y = (info.height - crop_h) / 2;
        }

        crop_w = crop_w ? crop_w : 1;
        crop_h = crop_h ? crop_h : 1;
        float scale_x = static_cast<float>(self->display_width) / static_cast<float>(crop_w);
        float scale_y = static_cast<float>(self->display_height) / static_cast<float>(crop_h);

        if (self->last_video_width != info.width || self->last_video_height != info.height) {
            ESP_LOGI(ESP_UTILS_LOG_TAG, "Cover video frame: %dx%d, crop %u,%u %ux%u",
                     info.width, info.height, crop_x, crop_y, crop_w, crop_h);
            self->last_video_width = info.width;
            self->last_video_height = info.height;
        }

        void *lcd_buffer = esp_lv_adapter_dummy_draw_get_free_buf(self->display);
        if (!lcd_buffer) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "No free LCD frame buffer");
            return;
        }

        const size_t frame_buffer_size = static_cast<size_t>(self->display_width) *
                                         self->display_height * self->video_bytes_per_pixel;
        const auto ppa_color_mode = self->display_rgb888 ?
                                    PPA_SRM_COLOR_MODE_RGB888 : PPA_SRM_COLOR_MODE_RGB565;

        ppa_srm_oper_config_t srm_config = {};
        srm_config.in.buffer = self->jpeg_output_buffer;
        srm_config.in.pic_w = info.width;
        srm_config.in.pic_h = info.height;
        srm_config.in.block_w = crop_w;
        srm_config.in.block_h = crop_h;
        srm_config.in.block_offset_x = crop_x;
        srm_config.in.block_offset_y = crop_y;
        srm_config.in.srm_cm = ppa_color_mode;

        srm_config.out.buffer = lcd_buffer;
        srm_config.out.buffer_size = ALIGN_UP(frame_buffer_size, self->data_cache_line_size);
        srm_config.out.pic_w = self->display_width;
        srm_config.out.pic_h = self->display_height;
        srm_config.out.block_offset_x = 0;
        srm_config.out.block_offset_y = 0;
        srm_config.out.srm_cm = ppa_color_mode;

        srm_config.rotation_angle = PPA_SRM_ROTATION_ANGLE_0;
        srm_config.scale_x = scale_x;
        srm_config.scale_y = scale_y;
        srm_config.mirror_x = 0;
        srm_config.mirror_y = 0;
        srm_config.rgb_swap = 0;
        srm_config.byte_swap = 0;
        srm_config.mode = PPA_TRANS_MODE_BLOCKING;

        esp_err_t ret = ppa_do_scale_rotate_mirror(self->ppa_srm_handle, &srm_config);
        if (ret != ESP_OK) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "PPA scale failed: %s", esp_err_to_name(ret));
            return;
        }

        if (self->display_rgb565_swapped) {
            uint8_t *pixels = static_cast<uint8_t *>(lcd_buffer);
            for (size_t i = 0; i + 1 < frame_buffer_size; i += 2) {
                const uint8_t byte = pixels[i];
                pixels[i] = pixels[i + 1];
                pixels[i + 1] = byte;
            }
        }

        ret = esp_lv_adapter_dummy_draw_flush_buf(self->display, lcd_buffer);
        if (ret != ESP_OK) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Dummy draw flush failed: %s", esp_err_to_name(ret));
            return;
        }
    }

    void VideoPlayer::audioCallback(frame_data_t *data, void *arg)
    {
        VideoPlayer *self = static_cast<VideoPlayer *>(arg);
        if (!self || !self->audio_ready || !self->audio_handles || !self->audio_handles->codec_dev || !self->loop_playback ||
                !data || data->type != FRAME_TYPE_AUDIO || !data->data || data->data_bytes == 0) {
            return;
        }

        if (data->data_bytes > static_cast<size_t>(INT_MAX)) {
            ESP_LOGW(ESP_UTILS_LOG_TAG, "Audio frame is too large: %u bytes",
                     static_cast<unsigned>(data->data_bytes));
            return;
        }
        const int ret = esp_codec_dev_write(
                            self->audio_handles->codec_dev,
                            data->data,
                            static_cast<int>(data->data_bytes)
                        );
        if (ret != ESP_CODEC_DEV_OK) {
            ESP_LOGW(
                ESP_UTILS_LOG_TAG,
                "Audio write failed: %d, %u bytes",
                ret,
                static_cast<unsigned>(data->data_bytes)
            );
        }
    }

    void VideoPlayer::audioSetClockCallback(uint32_t rate, uint32_t bits, uint32_t ch, void *arg)
    {
        VideoPlayer *self = static_cast<VideoPlayer *>(arg);
        if (!self || !self->audio_ready || !self->loop_playback) {
            return;
        }

        const esp_err_t ret = self->configureAudio(rate, bits, ch);
        if (ret != ESP_OK) {
            self->audio_ready = false;
            ESP_LOGW(ESP_UTILS_LOG_TAG, "Audio format setup failed: %s", esp_err_to_name(ret));
        }
    }

    void VideoPlayer::playEndCallback(void *arg)
    {
        VideoPlayer *self = static_cast<VideoPlayer *>(arg);
        self->is_playing = false;
    }

    bool VideoPlayer::shouldExitBySwipe()
    {
        if (!touch_handle) {
            return false;
        }

        esp_lcd_touch_read_data(touch_handle);

        esp_lcd_touch_point_data_t points[1] = {};
        uint8_t point_num = 0;
        esp_err_t ret = esp_lcd_touch_get_data(touch_handle, points, &point_num, 1);

        if (ret != ESP_OK || point_num == 0) {
            touch_active = false;
            return false;
        }

        if (!touch_active) {
            touch_active = true;
            touch_start_x = points[0].x;
            touch_start_y = points[0].y;
            return false;
        }

        int dx = abs((int)points[0].x - (int)touch_start_x);
        int dy = abs((int)points[0].y - (int)touch_start_y);
        return dx > SWIPE_EXIT_THRESHOLD || dy > SWIPE_EXIT_THRESHOLD;
    }

    void VideoPlayer::playTask(void *arg)
    {
        VideoPlayer *self = static_cast<VideoPlayer *>(arg);
        bool request_close = false;
        const esp_err_t audio_ret = self->initAudioDevice();
        if (audio_ret != ESP_OK) {
            ESP_LOGW(ESP_UTILS_LOG_TAG, "Audio DAC unavailable; continue video-only playback: %s",
                     esp_err_to_name(audio_ret));
        }


        // The AVI reader keeps a larger buffer because high-resolution MJPEG frames
        // can otherwise underflow while the PPA path is preparing the next frame.
        avi_player_config_t cfg = {
            .buffer_size = 2 * 1024 * 1024,
            .video_cb = videoCallback,
            .audio_cb = self->audio_ready ? audioCallback : nullptr,
            .audio_set_clock_cb = self->audio_ready ? audioSetClockCallback : nullptr,
            .avi_play_end_cb = playEndCallback,
            .priority = 7,
            .coreID = 0,
            .user_data = self,
            .stack_size = 32768,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
            .stack_in_psram = true,
#endif
        };

        if (avi_player_init(cfg, &self->avi_player_handle) != ESP_OK)
        {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "avi_player_init failed");
            self->deinitAudioDevice();
            self->play_task_handle = nullptr;
            if (self->close_wait_task) {
                xTaskNotifyGive(self->close_wait_task);
            }
            vTaskDelete(NULL);
            return;
        }

        if (self->initDisplayBypass() != ESP_OK)
        {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "initDisplayBypass failed");
            avi_player_deinit(self->avi_player_handle);
            self->avi_player_handle = nullptr;
            self->deinitDisplayBypass();
            self->deinitAudioDevice();
            self->play_task_handle = nullptr;
            if (self->close_wait_task) {
                xTaskNotifyGive(self->close_wait_task);
            }
            vTaskDelete(NULL);
            return;
        }

        // Loop through every AVI file discovered on the SD card until the app closes.
        while (self->loop_playback)
        {
            for (int i = 0; i < self->avi_file_count && self->loop_playback; i++)
            {
                while (self->is_paused && self->loop_playback)
                {
                    if (self->shouldExitBySwipe()) {
                        request_close = true;
                        self->loop_playback = false;
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(100));
                }

                if (!self->loop_playback)
                    break;

                ESP_LOGI(ESP_UTILS_LOG_TAG, "Playing AVI file: %s", self->avi_file_list[i]);
                self->is_playing = true;
                if (avi_player_play_from_file(self->avi_player_handle, self->avi_file_list[i]) != ESP_OK) {
                    ESP_LOGE(ESP_UTILS_LOG_TAG, "Failed to play file: %s", self->avi_file_list[i]);
                    self->is_playing = false;
                    continue;
                }

                while (self->is_playing && self->loop_playback)
                {
                    if (self->shouldExitBySwipe()) {
                        request_close = true;
                        self->loop_playback = false;
                        avi_player_play_stop(self->avi_player_handle);
                        self->is_playing = false;
                        break;
                    }

                    if (self->is_paused)
                    {
                        avi_player_play_stop(self->avi_player_handle);
                        self->is_playing = false;
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(30));
                }
            }

            vTaskDelay(pdMS_TO_TICKS(500));
        }

        avi_player_play_stop(self->avi_player_handle);
        avi_player_deinit(self->avi_player_handle);
        self->avi_player_handle = nullptr;
        self->deinitAudioDevice();
        self->deinitDisplayBypass();

        self->play_task_handle = nullptr;
        if (self->close_wait_task)
        {
            xTaskNotifyGive(self->close_wait_task);
        }

        if (request_close)
        {
            self->notifyCoreClosed();
        }

        vTaskDelete(NULL);
    }

} // namespace esp_brookesia::apps
