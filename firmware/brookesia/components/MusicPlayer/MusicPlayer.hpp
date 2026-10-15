/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once
#include <memory>
#include <string>
#include <vector>
#include "systems/phone/esp_brookesia_phone_app.hpp"
#include "src/lv_demo_music.h"
#include "esp_err.h"
namespace esp_brookesia::apps {
class MusicPlayer: public systems::phone::App {
public:
    static MusicPlayer *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);
    ~MusicPlayer();
protected:
    MusicPlayer(bool use_status_bar, bool use_navigation_bar);
    bool run() override;
    bool back() override;
    bool close() override;
    bool init() override;
    bool deinit() override;
    bool pause() override;
    bool resume() override;
private:
    class PlaybackStateMonitor;
    static MusicPlayer *_instance;
    static const char *title_cb(void *context, uint32_t id);
    static bool play_cb(void *context, uint32_t id);
    static bool pause_cb(void *context);
    static bool resume_cb(void *context, uint32_t id);
    static app_music_state_t status_cb(void *context, uint32_t *position, uint32_t *duration);
    esp_err_t cleanup_player();
    void show_message(const char *message, esp_err_t error = ESP_OK);
    std::unique_ptr<PlaybackStateMonitor> _state_monitor;
    std::vector<std::string> _tracks;
    lv_obj_t *_message = nullptr;
    uint32_t _loaded_track = UINT32_MAX;
    uint32_t _play_request_ms = 0;
    uint32_t _play_started_ms = 0;
    uint32_t _elapsed_ms = 0;
    bool _play_pending = false;
    bool _playback_started = false;
    bool _is_counting = false;
    bool _spiffs_owned = false;
    bool _ui_active = false;
};
}
