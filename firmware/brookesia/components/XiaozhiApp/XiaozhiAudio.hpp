/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace esp_brookesia::apps {

class XiaozhiAudio {
public:
    enum class CaptureMode { Off, WakeWord, Voice };
    using EncodedCallback = std::function<void(const uint8_t *, size_t)>;
    using WakeCallback = std::function<void(const std::string &)>;

    XiaozhiAudio();
    ~XiaozhiAudio();
    XiaozhiAudio(const XiaozhiAudio &) = delete;
    XiaozhiAudio &operator=(const XiaozhiAudio &) = delete;

    // Lifecycle and capture control are serialized by the app's control task.
    bool open(EncodedCallback encoded = {}, WakeCallback wake = {});
    void close();
    bool isReady() const;
    bool setCapture(CaptureMode mode);

    // Network submissions may run concurrently with playback reset or close.
    bool writeOpus(const uint8_t *data, size_t size, uint32_t sample_rate, uint32_t frame_ms,
                   const std::function<bool()> &accept = {});
    bool resetPlayback();
    bool isPlaybackDrained() const;

private:
    struct Session;
    std::atomic<std::shared_ptr<Session>> session_;
};
} // namespace esp_brookesia::apps
