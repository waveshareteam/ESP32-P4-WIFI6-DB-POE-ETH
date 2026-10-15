/*
 * SPDX-FileCopyrightText: 2026 Waveshare Electronics
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <map>
#include <mutex>

#include "brookesia/hal_interface/interfaces/audio/codec_player.hpp"
#include "brookesia/hal_interface/interfaces/audio/codec_recorder.hpp"

namespace esp_hal_boards {

class AudioCodecPlayer final : public esp_brookesia::hal::audio::CodecPlayerIface {
public:
    AudioCodecPlayer();
    ~AudioCodecPlayer() override;

    bool is_valid() const;
    bool open(const Config &config) override;
    void close() override;
    bool set_volume(uint8_t volume) override;
    bool write_data(const uint8_t *data, size_t size) override;
    bool is_pa_on_off_supported() override;
    bool set_pa_on_off(bool on) override;
    bool is_pa_on() const override;

private:
    bool set_pa_on_off_locked(bool on);

    mutable std::mutex mutex_;
    void *codec_handles_ = nullptr;
    void *pa_control_handle_ = nullptr;
    uint8_t volume_ = 75;
    bool pa_active_level_ = true;
    bool codec_opened_ = false;
    bool pa_is_on_ = false;
};

class AudioCodecRecorder final : public esp_brookesia::hal::audio::CodecRecorderIface {
public:
    explicit AudioCodecRecorder(Info info);
    ~AudioCodecRecorder() override;

    bool is_valid() const;
    bool open() override;
    void close() override;
    bool read_data(uint8_t *data, size_t size) override;
    bool set_general_gain(float gain) override;
    bool set_channel_gains(const std::map<uint8_t, float> &gains) override;

private:
    bool apply_gains_locked();

    mutable std::mutex mutex_;
    void *codec_handles_ = nullptr;
    bool codec_opened_ = false;
    float general_gain_ = 36.0f;
    std::map<uint8_t, float> channel_gains_;
};

} // namespace esp_hal_boards
