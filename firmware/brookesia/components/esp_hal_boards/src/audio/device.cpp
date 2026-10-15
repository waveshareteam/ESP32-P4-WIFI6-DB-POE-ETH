/*
 * SPDX-FileCopyrightText: 2026 Waveshare Electronics
 * SPDX-License-Identifier: Apache-2.0
 */
#include "esp_hal_boards/config.h"

#include <utility>

#define BROOKESIA_LOG_TAG "EspHalBoardsAudio"
#include "brookesia/lib_utils/check.hpp"
#include "esp_hal_boards/audio_device.hpp"

#include <cstdint>
#include "codec_interfaces.hpp"
#include "processor_impl.hpp"

namespace esp_hal_boards {
namespace hal = esp_brookesia::hal;
namespace audio = esp_brookesia::hal::audio;

constexpr uint32_t AUDIO_BUS_SAMPLE_RATE = 48000;

AudioDevice &AudioDevice::get_instance()
{
    static AudioDevice instance;
    return instance;
}

AudioDevice::AudioDevice()
    : Device(DEVICE_NAME)
{
    recorder_info_ = audio::CodecRecorderIface::Info {
        .bits = 16,
        .channels = 2,
        .sample_rate = AUDIO_BUS_SAMPLE_RATE,
        .mic_layout = "FC,RE",
        .general_gain = 36.0f,
        .channel_gains = {},
    };
    // Keep the shared I2S clock aligned with the audio player's 48 kHz output.
    processor_config_.playback.player.sample_rate = AUDIO_BUS_SAMPLE_RATE;
}

bool AudioDevice::set_codec_recorder_info(audio::CodecRecorderIface::Info info)
{
    std::lock_guard lock(config_mutex_);
    if (initialized_ || info.bits == 0 || info.channels == 0 || info.sample_rate == 0 || info.mic_layout.empty()) {
        return false;
    }
    recorder_info_ = std::move(info);
    return true;
}

bool AudioDevice::set_processor_config(hal::AudioProcessorConfig config)
{
    std::lock_guard lock(config_mutex_);
    if (initialized_) {
        return false;
    }
    processor_config_ = std::move(config);
    return true;
}

bool AudioDevice::set_afe_input_layout(std::string layout)
{
    std::lock_guard lock(config_mutex_);
    if (initialized_ || layout.empty()) {
        return false;
    }
    afe_input_layout_ = std::move(layout);
    return true;
}

bool AudioDevice::probe()
{
    return true;
}

std::vector<hal::InterfaceSpec> AudioDevice::get_interface_specs() const
{
    return {
        {audio::CodecPlayerIface::NAME, CODEC_PLAYER_NAME},
        {audio::CodecRecorderIface::NAME, CODEC_RECORDER_NAME},
#if CONFIG_ESP_HAL_BOARDS_AUDIO_ENABLE_PROCESSOR
        {audio::PlaybackIface::NAME, PLAYBACK_NAME},
        {audio::EncoderIface::NAME, ENCODER_NAME},
        {audio::DecoderIface::NAME, DECODER_NAME},
#endif
    };
}

bool AudioDevice::on_init()
{
    std::lock_guard lock(config_mutex_);
    if (initialized_) {
        return true;
    }

    std::shared_ptr<AudioCodecPlayer> player;
    BROOKESIA_CHECK_EXCEPTION_RETURN(
        player = std::make_shared<AudioCodecPlayer>(), false, "Failed to create codec player interface"
    );
    if (!player->is_valid()) {
        return false;
    }
    std::shared_ptr<AudioCodecRecorder> recorder;
    BROOKESIA_CHECK_EXCEPTION_RETURN(
        recorder = std::make_shared<AudioCodecRecorder>(recorder_info_.value()), false,
        "Failed to create codec recorder interface"
    );
    if (!recorder->is_valid()) {
        return false;
    }

#if CONFIG_ESP_HAL_BOARDS_AUDIO_ENABLE_PROCESSOR
    std::shared_ptr<AudioProcessorCore> processor_core;
    std::shared_ptr<AudioPlaybackImpl> playback;
    std::shared_ptr<AudioEncoderImpl> encoder;
    std::shared_ptr<AudioDecoderImpl> decoder;
    BROOKESIA_CHECK_EXCEPTION_RETURN(
        processor_core = std::make_shared<AudioProcessorCore>(player, recorder, processor_config_, afe_input_layout_),
        false, "Failed to create audio processor core"
    );
    BROOKESIA_CHECK_EXCEPTION_RETURN(
        playback = std::make_shared<AudioPlaybackImpl>(processor_core), false,
        "Failed to create playback interface"
    );
    BROOKESIA_CHECK_EXCEPTION_RETURN(
        encoder = std::make_shared<AudioEncoderImpl>(processor_core), false,
        "Failed to create encoder interface"
    );
    BROOKESIA_CHECK_EXCEPTION_RETURN(
        decoder = std::make_shared<AudioDecoderImpl>(processor_core), false,
        "Failed to create decoder interface"
    );
#endif

    interfaces_.emplace(CODEC_PLAYER_NAME, player);
    interfaces_.emplace(CODEC_RECORDER_NAME, recorder);
#if CONFIG_ESP_HAL_BOARDS_AUDIO_ENABLE_PROCESSOR
    processor_core_ = std::move(processor_core);
    interfaces_.emplace(PLAYBACK_NAME, std::move(playback));
    interfaces_.emplace(ENCODER_NAME, std::move(encoder));
    interfaces_.emplace(DECODER_NAME, std::move(decoder));
#endif

    initialized_ = true;
    return true;
}

void AudioDevice::on_deinit()
{
    std::lock_guard lock(config_mutex_);
    interfaces_.clear();
    processor_core_.reset();
    initialized_ = false;
}

} // namespace esp_hal_boards

#if CONFIG_ESP_HAL_BOARDS_AUDIO_ENABLE
BROOKESIA_PLUGIN_REGISTER_SINGLETON_WITH_SYMBOL(
    esp_brookesia::hal::Device, esp_hal_boards::AudioDevice, esp_hal_boards::AudioDevice::DEVICE_NAME,
    esp_hal_boards::AudioDevice::get_instance(), ESP_HAL_BOARDS_AUDIO_DEVICE_PLUGIN_SYMBOL
);
#endif
