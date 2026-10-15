/* SPDX-License-Identifier: Apache-2.0 */
#include "XiaozhiAudio.hpp"

#include <mutex>
#include <span>
#include <utility>

#include "brookesia/service_manager/dataflow/registry.hpp"
#include "brookesia/service_manager/service/manager.hpp"
#include "esp_hal_boards/audio_device.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace esp_brookesia::apps {
namespace {
namespace df = service::dataflow;
constexpr char TAG[] = "XiaozhiAudio";
constexpr char OUTPUT[] = "Speaker0";
constexpr uint32_t PLAYBACK_TAIL_MS = 200;

df::AudioCaptureConfig voiceConfig()
{
    df::AudioCaptureConfig config{};
    config.type = df::AudioCodecFormat::OPUS;
    config.general = {1, 16, 16000, 60};
    config.opus = df::AudioEncoderExtraConfigOpus{true, 24000};
    config.enable_afe = true;
    config.fetch_data_size = 1500;
    config.afe_wake_start_timeout_ms = 0;
    config.afe_wake_end_timeout_ms = 0;
    return config;
}

bool validFormat(uint32_t rate, uint32_t duration)
{
    return (rate == 8000 || rate == 12000 || rate == 16000 || rate == 24000 || rate == 48000) &&
           (duration == 5 || duration == 10 || duration == 20 || duration == 40 ||
            duration == 60 || duration == 80 || duration == 100 || duration == 120);
}
} // namespace

struct XiaozhiAudio::Session {
    std::shared_ptr<df::AudioPlaybackOperation> playback;
    std::shared_ptr<df::AudioCaptureOperation> capture;
    lib_utils::connection data_connection;
    lib_utils::connection wake_connection;

    // Control calls run on the app's control task. These two locks only guard
    // callbacks in flight and playback submissions from the network task.
    std::mutex callback_mutex;
    mutable std::mutex playback_mutex;
    EncodedCallback encoded;
    WakeCallback wake;
    CaptureMode mode = CaptureMode::Off;
    std::string wake_word;
    uint32_t rate = 0;
    uint32_t frame_ms = 0;
    TickType_t estimated_playback_end = 0;

    ~Session() { shutdown(); }

    bool openPlayback(df::DataFlowRegistry &registry, const char *owner)
    {
        df::AudioPlaybackOperationConfig config{};
        config.owner = owner;
        config.provider_id = "audio.decoder.0";
        config.source.name = owner;
        config.source.role = "voice";
        config.output_name = OUTPUT;
        config.request_output = true;
        config.activate_source = true;
        auto result = registry.open_audio_playback_operation(std::move(config));
        if (!result) {
            ESP_LOGE(TAG, "Open playback: %s", result.error().c_str());
            return false;
        }
        playback = *result;
        return true;
    }

    void shutdown()
    {
        {
            std::lock_guard lock(callback_mutex);
            mode = CaptureMode::Off;
            encoded = {};
            wake = {};
        }
        data_connection.disconnect();
        wake_connection.disconnect();
        if (capture) capture->close();
        std::lock_guard lock(playback_mutex);
        if (playback) playback->close();
    }

    bool openStream(uint32_t sample_rate, uint32_t duration)
    {
        if (rate == sample_rate && frame_ms == duration) return true;
        if (rate && !playback->close_stream(OUTPUT)) return false;
        rate = 0;
        df::AudioStreamConfig config{};
        config.type = df::AudioCodecFormat::OPUS;
        config.general = {1, 16, sample_rate, static_cast<uint8_t>(duration)};
        config.queue_size_bytes = 64 * 1024;
        auto result = playback->open_stream(OUTPUT, config);
        if (!result) {
            ESP_LOGE(TAG, "Open Opus stream: %s", result.error().c_str());
            return false;
        }
        rate = sample_rate;
        frame_ms = duration;
        return true;
    }
};

XiaozhiAudio::XiaozhiAudio() = default;
XiaozhiAudio::~XiaozhiAudio() { close(); }

bool XiaozhiAudio::open(EncodedCallback encoded, WakeCallback wake)
{
    close();
    auto &manager = service::ServiceManager::get_instance();
    if (!manager.is_initialized() && !manager.init()) return false;
    if (!manager.is_running() && !manager.start()) return false;

    auto session = std::make_shared<Session>();
    auto &registry = manager.get_dataflow_registry();
    if (!session->openPlayback(registry, encoded ? "XiaozhiApp" : "XiaozhiPrompt")) return false;
    if (encoded) {
        df::AudioCaptureOperationConfig config{};
        config.owner = "XiaozhiApp";
        config.provider_id = "audio.encoder.0";
        auto result = registry.open_audio_capture_operation(std::move(config));
        if (!result) {
            ESP_LOGE(TAG, "Open capture: %s", result.error().c_str());
            return false;
        }
        session->capture = *result;
        session->encoded = std::move(encoded);
        session->wake = std::move(wake);
        auto weak = std::weak_ptr<Session>(session);
        session->data_connection = session->capture->connect_data(
            [weak](std::span<const uint8_t> data) {
                if (auto session = weak.lock()) {
                    std::lock_guard lock(session->callback_mutex);
                    if (session->mode == CaptureMode::Voice && session->encoded)
                        session->encoded(data.data(), data.size());
                }
            });
        session->wake_connection = session->capture->connect_afe_event(
            [weak](df::AudioAfeEvent event) {
                if (event != df::AudioAfeEvent::WakeStart) return;
                ESP_LOGI(TAG, "WakeNet WakeStart received");
                if (auto session = weak.lock()) {
                    std::lock_guard lock(session->callback_mutex);
                    if (session->mode == CaptureMode::WakeWord && session->wake)
                        session->wake(session->wake_word);
                }
            });
    }
    session_.store(std::move(session));
    return true;
}

void XiaozhiAudio::close()
{
    if (auto session = session_.exchange(nullptr)) session->shutdown();
}

bool XiaozhiAudio::isReady() const
{
    auto session = session_.load();
    return session && session->playback->is_available();
}

bool XiaozhiAudio::setCapture(CaptureMode mode)
{
    auto session = session_.load();
    if (!session || !session->capture) return mode == CaptureMode::Off;
    {
        std::lock_guard lock(session->callback_mutex);
        if (session->mode == mode &&
            (mode == CaptureMode::Off || session->capture->is_started())) return true;
        session->mode = CaptureMode::Off;
    }
    session->capture->stop();
    if (mode == CaptureMode::Off) return true;

    hal::AudioProcessorAFE_Config afe{};
    afe.vad = hal::AudioProcessorAFE_VAD_Config{};
    if (mode == CaptureMode::WakeWord)
        afe.wakenet = hal::AudioProcessorAFE_WakeNetConfig{};
    if (!esp_hal_boards::AudioDevice::get_instance().set_encoder_afe_config(std::move(afe)))
        return false;

    auto result = session->capture->start(voiceConfig());
    if (!result) {
        ESP_LOGE(TAG, "Start capture: %s", result.error().c_str());
        return false;
    }
    std::string wake_word;
    if (mode == CaptureMode::WakeWord) {
        auto words = session->capture->get_afe_wake_words();
        if (words.empty()) {
            session->capture->stop();
            ESP_LOGE(TAG, "No wake words configured");
            return false;
        }
        wake_word = words.front();
    }
    std::lock_guard lock(session->callback_mutex);
    session->wake_word = std::move(wake_word);
    session->mode = mode;
    ESP_LOGI(TAG, "Capture ready: %s", mode == CaptureMode::WakeWord ? "WakeWord" : "Voice");
    return true;
}

bool XiaozhiAudio::writeOpus(
    const uint8_t *data, size_t size, uint32_t rate, uint32_t frame_ms,
    const std::function<bool()> &accept)
{
    if (!data || !size || size > 1500 || !validFormat(rate, frame_ms)) return false;
    auto session = session_.load();
    if (!session) return false;
    std::lock_guard lock(session->playback_mutex);
    if (!session->playback->is_available()) return false;
    if (accept && !accept()) return true;
    if (!session->openStream(rate, frame_ms)) return false;
    if (session->playback->write_copy(OUTPUT, {data, size}, 0) != df::AudioWriteResult::Written)
        return false;

    // DataFlow reports an empty input queue, not speaker completion. Keep the
    // existing duration estimate and tail allowance until HAL exposes drain.
    auto now = xTaskGetTickCount();
    if (static_cast<int32_t>(session->estimated_playback_end - now) < 0)
        session->estimated_playback_end = now;
    session->estimated_playback_end += pdMS_TO_TICKS(frame_ms);
    return true;
}

bool XiaozhiAudio::resetPlayback()
{
    auto session = session_.load();
    if (!session) return true;
    std::lock_guard lock(session->playback_mutex);
    if (session->rate && !session->playback->close_stream(OUTPUT)) return false;
    session->rate = 0;
    session->frame_ms = 0;
    session->estimated_playback_end = 0;
    return true;
}

bool XiaozhiAudio::isPlaybackDrained() const
{
    auto session = session_.load();
    if (!session) return true;
    std::lock_guard lock(session->playback_mutex);
    if (!session->rate) return true;
    if (!session->playback->is_stream_drained(OUTPUT)) return false;
    return static_cast<int32_t>(xTaskGetTickCount() - session->estimated_playback_end) >=
           static_cast<int32_t>(pdMS_TO_TICKS(PLAYBACK_TAIL_MS));
}
} // namespace esp_brookesia::apps
