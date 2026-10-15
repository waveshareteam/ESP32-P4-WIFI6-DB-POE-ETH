/*
 * SPDX-FileCopyrightText: 2026 Waveshare Electronics
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "brookesia/hal_adaptor/audio/device.hpp"
#include "brookesia/hal_interface/device.hpp"
#include "brookesia/hal_interface/interfaces/audio/codec_player.hpp"
#include "brookesia/hal_interface/interfaces/audio/codec_recorder.hpp"
#include "brookesia/hal_interface/interfaces/audio/processor.hpp"

namespace esp_hal_boards {

class AudioCodecPlayer;
class AudioCodecRecorder;
class AudioProcessorCore;

/** Board Manager-backed audio HAL provider for the Waveshare POE-ETH board. */
class AudioDevice final : public esp_brookesia::hal::Device {
public:
    static constexpr const char *DEVICE_NAME = "WaveshareAudio";
    static constexpr const char *CODEC_PLAYER_NAME = "Audio:CodecPlayer";
    static constexpr const char *CODEC_RECORDER_NAME = "Audio:CodecRecorder";
    static constexpr const char *PLAYBACK_NAME = "Audio:Playback";
    static constexpr const char *ENCODER_NAME = "Audio:Encoder:0";
    static constexpr const char *DECODER_NAME = "Audio:Decoder:0";

    AudioDevice(const AudioDevice &) = delete;
    AudioDevice &operator=(const AudioDevice &) = delete;
    AudioDevice(AudioDevice &&) = delete;
    AudioDevice &operator=(AudioDevice &&) = delete;

    static AudioDevice &get_instance();

    bool set_codec_recorder_info(esp_brookesia::hal::audio::CodecRecorderIface::Info info);
    bool set_processor_config(esp_brookesia::hal::AudioProcessorConfig config);
    bool set_afe_input_layout(std::string layout);

private:
    AudioDevice();
    ~AudioDevice() = default;

    bool probe() override;
    std::vector<esp_brookesia::hal::InterfaceSpec> get_interface_specs() const override;
    bool on_init() override;
    void on_deinit() override;

    mutable std::mutex config_mutex_;
    bool initialized_ = false;
    std::optional<esp_brookesia::hal::audio::CodecRecorderIface::Info> recorder_info_;
    esp_brookesia::hal::AudioProcessorConfig processor_config_{};
    std::string afe_input_layout_ = "MR";
    std::shared_ptr<AudioProcessorCore> processor_core_;
};

} // namespace esp_hal_boards
