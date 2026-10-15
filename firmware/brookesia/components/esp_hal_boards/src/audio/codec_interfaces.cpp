/*
 * SPDX-FileCopyrightText: 2026 Waveshare Electronics
 * SPDX-License-Identifier: Apache-2.0
 */
#include "codec_interfaces.hpp"

#include <algorithm>
#include <climits>
#include <cinttypes>
#include <utility>

#include "esp_board_manager_includes.h"
#include "driver/gpio.h"
#include "esp_codec_dev.h"
#include "esp_log.h"

namespace esp_hal_boards {
namespace {

constexpr char TAG[] = "EspHalBoardsAudio";

esp_codec_dev_handle_t codec_handle(void *handles)
{
    if (handles == nullptr) {
        return nullptr;
    }
    return static_cast<dev_audio_codec_handles_t *>(handles)->codec_dev;
}

bool valid_sample_config(uint8_t bits, uint8_t channels, uint32_t sample_rate)
{
    return (bits == 16 || bits == 24 || bits == 32) && channels > 0 && sample_rate > 0;
}

} // namespace

AudioCodecPlayer::AudioCodecPlayer()
    : CodecPlayerIface()
{
    if (!esp_board_manager_check_name(ESP_BOARD_DEVICE_NAME_AUDIO_DAC)) {
        ESP_LOGE(TAG, "Board Manager audio_dac device is not configured");
        return;
    }

    esp_err_t ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_AUDIO_DAC);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Initialize audio_dac failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_AUDIO_DAC, &codec_handles_);
    if (ret != ESP_OK || codec_handle(codec_handles_) == nullptr) {
        ESP_LOGE(TAG, "Get audio_dac codec handle failed: %s", esp_err_to_name(ret));
        (void)esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_AUDIO_DAC);
        codec_handles_ = nullptr;
        return;
    }

    if (esp_board_manager_check_name(ESP_BOARD_PERIPH_NAME_GPIO_PA_CONTROL)) {
        ret = esp_board_manager_get_periph_handle(
                  ESP_BOARD_PERIPH_NAME_GPIO_PA_CONTROL, &pa_control_handle_
              );
        dev_audio_codec_config_t *config = nullptr;
        if (ret == ESP_OK) {
            ret = esp_board_manager_get_device_config(
                      ESP_BOARD_DEVICE_NAME_AUDIO_DAC, reinterpret_cast<void **>(&config)
                  );
        }
        if (ret != ESP_OK || config == nullptr || pa_control_handle_ == nullptr) {
            ESP_LOGE(TAG, "Get PA control configuration failed: %s", esp_err_to_name(ret));
            pa_control_handle_ = nullptr;
            (void)esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_AUDIO_DAC);
            codec_handles_ = nullptr;
            return;
        }
        pa_active_level_ = !config->codec_pa_cfg.pa_active_low;
        if (!set_pa_on_off_locked(false)) {
            ESP_LOGE(TAG, "Disable PA after audio_dac initialization failed");
            pa_control_handle_ = nullptr;
            (void)esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_AUDIO_DAC);
            codec_handles_ = nullptr;
        }
    }
}

AudioCodecPlayer::~AudioCodecPlayer()
{
    close();
    std::lock_guard lock(mutex_);
    if (codec_handles_ != nullptr) {
        const esp_err_t ret = esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_AUDIO_DAC);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Release audio_dac failed: %s", esp_err_to_name(ret));
        }
        codec_handles_ = nullptr;
        pa_control_handle_ = nullptr;
    }
}

bool AudioCodecPlayer::is_valid() const
{
    std::lock_guard lock(mutex_);
    return codec_handle(codec_handles_) != nullptr;
}

bool AudioCodecPlayer::open(const Config &config)
{
    std::lock_guard lock(mutex_);
    if (codec_opened_) {
        return true;
    }
    if (!valid_sample_config(config.bits, config.channels, config.sample_rate)) {
        ESP_LOGE(TAG, "Invalid playback format: %u-bit, %u channels, %" PRIu32 " Hz",
                 config.bits, config.channels, config.sample_rate);
        return false;
    }
    const auto handle = codec_handle(codec_handles_);
    if (handle == nullptr) {
        return false;
    }

    esp_codec_dev_sample_info_t sample_info = {
        .bits_per_sample = config.bits,
        .channel = config.channels,
        .sample_rate = config.sample_rate,
    };
    ESP_LOGI(TAG, "Open playback codec: %" PRIu32 " Hz, %u-bit, %u channels",
             config.sample_rate, config.bits, config.channels);
    esp_err_t ret = esp_codec_dev_open(handle, &sample_info);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Open playback codec failed: %d", ret);
        return false;
    }
    codec_opened_ = true;

    ret = esp_codec_dev_set_out_vol(handle, std::clamp<uint8_t>(volume_, 0, 100));
    if (ret == ESP_CODEC_DEV_OK) {
        ret = esp_codec_dev_set_out_mute(handle, false);
    }
    if (ret != ESP_CODEC_DEV_OK || (pa_control_handle_ != nullptr && !set_pa_on_off_locked(true))) {
        ESP_LOGE(TAG, "Configure playback output after open failed");
        (void)esp_codec_dev_close(handle);
        codec_opened_ = false;
        (void)set_pa_on_off_locked(false);
        return false;
    }
    return true;
}

void AudioCodecPlayer::close()
{
    std::lock_guard lock(mutex_);
    if (codec_opened_) {
        const esp_err_t ret = esp_codec_dev_close(codec_handle(codec_handles_));
        if (ret != ESP_CODEC_DEV_OK) {
            ESP_LOGW(TAG, "Close playback codec failed: %d", ret);
        }
        codec_opened_ = false;
    }
    if (pa_control_handle_ != nullptr && !set_pa_on_off_locked(false)) {
        ESP_LOGW(TAG, "Disable PA failed while closing playback");
    }
}

bool AudioCodecPlayer::set_volume(uint8_t volume)
{
    std::lock_guard lock(mutex_);
    volume_ = std::clamp<uint8_t>(volume, 0, 100);
    if (!codec_opened_) {
        return true;
    }
    return esp_codec_dev_set_out_vol(codec_handle(codec_handles_), volume_) == ESP_CODEC_DEV_OK;
}

bool AudioCodecPlayer::write_data(const uint8_t *data, size_t size)
{
    std::lock_guard lock(mutex_);
    if (!codec_opened_ || data == nullptr || size == 0 || size > INT_MAX) {
        return false;
    }
    return esp_codec_dev_write(
               codec_handle(codec_handles_), const_cast<uint8_t *>(data), static_cast<int>(size)
           ) == ESP_CODEC_DEV_OK;
}

bool AudioCodecPlayer::is_pa_on_off_supported()
{
    std::lock_guard lock(mutex_);
    return pa_control_handle_ != nullptr;
}

bool AudioCodecPlayer::set_pa_on_off(bool on)
{
    std::lock_guard lock(mutex_);
    return set_pa_on_off_locked(on);
}

bool AudioCodecPlayer::is_pa_on() const
{
    std::lock_guard lock(mutex_);
    return pa_is_on_;
}

bool AudioCodecPlayer::set_pa_on_off_locked(bool on)
{
    if (pa_control_handle_ == nullptr) {
        return !on;
    }
    auto *gpio = static_cast<periph_gpio_handle_t *>(pa_control_handle_);
    const int level = on ? static_cast<int>(pa_active_level_) : static_cast<int>(!pa_active_level_);
    const esp_err_t ret = gpio_set_level(gpio->gpio_num, level);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Set PA GPIO level failed: %s", esp_err_to_name(ret));
        return false;
    }
    pa_is_on_ = on;
    return true;
}

AudioCodecRecorder::AudioCodecRecorder(Info info)
    : CodecRecorderIface(std::move(info))
    , general_gain_(get_info().general_gain)
    , channel_gains_(get_info().channel_gains)
{
    if (!esp_board_manager_check_name(ESP_BOARD_DEVICE_NAME_AUDIO_ADC)) {
        ESP_LOGE(TAG, "Board Manager audio_adc device is not configured");
        return;
    }
    const esp_err_t ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_AUDIO_ADC);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Initialize audio_adc failed: %s", esp_err_to_name(ret));
        return;
    }
    esp_err_t handle_ret = esp_board_manager_get_device_handle(
                               ESP_BOARD_DEVICE_NAME_AUDIO_ADC, &codec_handles_
                           );
    if (handle_ret != ESP_OK || codec_handle(codec_handles_) == nullptr) {
        ESP_LOGE(TAG, "Get audio_adc codec handle failed: %s", esp_err_to_name(handle_ret));
        (void)esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_AUDIO_ADC);
        codec_handles_ = nullptr;
    }
}

AudioCodecRecorder::~AudioCodecRecorder()
{
    close();
    std::lock_guard lock(mutex_);
    if (codec_handles_ != nullptr) {
        const esp_err_t ret = esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_AUDIO_ADC);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Release audio_adc failed: %s", esp_err_to_name(ret));
        }
        codec_handles_ = nullptr;
    }
}

bool AudioCodecRecorder::is_valid() const
{
    std::lock_guard lock(mutex_);
    return codec_handle(codec_handles_) != nullptr;
}

bool AudioCodecRecorder::open()
{
    std::lock_guard lock(mutex_);
    if (codec_opened_) {
        return true;
    }
    const auto &info = get_info();
    if (!valid_sample_config(info.bits, info.channels, info.sample_rate)) {
        ESP_LOGE(TAG, "Invalid capture format: %u-bit, %u channels, %" PRIu32 " Hz",
                 info.bits, info.channels, info.sample_rate);
        return false;
    }
    const auto handle = codec_handle(codec_handles_);
    if (handle == nullptr) {
        return false;
    }

    esp_codec_dev_sample_info_t sample_info = {
        .bits_per_sample = info.bits,
        .channel = info.channels,
        .sample_rate = info.sample_rate,
    };
    esp_err_t ret = esp_codec_dev_open(handle, &sample_info);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Open capture codec failed: %d", ret);
        return false;
    }
    codec_opened_ = true;

    if (!info.mic_layout.empty()) {
        ret = esp_codec_dev_set_data_layout_label(handle, info.mic_layout.c_str());
    }
    if (ret != ESP_CODEC_DEV_OK || !apply_gains_locked()) {
        ESP_LOGE(TAG, "Configure capture codec after open failed");
        (void)esp_codec_dev_close(handle);
        codec_opened_ = false;
        return false;
    }
    return true;
}

void AudioCodecRecorder::close()
{
    std::lock_guard lock(mutex_);
    if (!codec_opened_) {
        return;
    }
    const esp_err_t ret = esp_codec_dev_close(codec_handle(codec_handles_));
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGW(TAG, "Close capture codec failed: %d", ret);
    }
    codec_opened_ = false;
}

bool AudioCodecRecorder::read_data(uint8_t *data, size_t size)
{
    std::lock_guard lock(mutex_);
    if (!codec_opened_ || data == nullptr || size == 0 || size > INT_MAX) {
        return false;
    }
    return esp_codec_dev_read(codec_handle(codec_handles_), data, static_cast<int>(size)) == ESP_CODEC_DEV_OK;
}

bool AudioCodecRecorder::set_general_gain(float gain)
{
    std::lock_guard lock(mutex_);
    if (!codec_opened_) {
        general_gain_ = gain;
        return true;
    }
    if (esp_codec_dev_set_in_gain(codec_handle(codec_handles_), gain) != ESP_CODEC_DEV_OK) {
        return false;
    }
    general_gain_ = gain;
    return true;
}

bool AudioCodecRecorder::set_channel_gains(const std::map<uint8_t, float> &gains)
{
    std::lock_guard lock(mutex_);
    const auto &info = get_info();
    for (const auto &[channel, gain] : gains) {
        if (channel >= info.channels || channel >= sizeof(uint16_t) * CHAR_BIT) {
            return false;
        }
    }
    if (codec_opened_) {
        for (const auto &[channel, gain] : gains) {
            const esp_err_t ret = esp_codec_dev_set_in_channel_gain(
                                      codec_handle(codec_handles_), static_cast<uint16_t>(1U << channel), gain
                                  );
            if (ret != ESP_CODEC_DEV_OK) {
                return false;
            }
        }
    }
    channel_gains_ = gains;
    return true;
}

bool AudioCodecRecorder::apply_gains_locked()
{
    if (esp_codec_dev_set_in_gain(codec_handle(codec_handles_), general_gain_) != ESP_CODEC_DEV_OK) {
        return false;
    }
    const auto &info = get_info();
    for (const auto &[channel, gain] : channel_gains_) {
        if (channel >= info.channels || channel >= sizeof(uint16_t) * CHAR_BIT) {
            return false;
        }
        if (esp_codec_dev_set_in_channel_gain(
                codec_handle(codec_handles_), static_cast<uint16_t>(1U << channel), gain
            ) != ESP_CODEC_DEV_OK) {
            return false;
        }
    }
    return true;
}

} // namespace esp_hal_boards
