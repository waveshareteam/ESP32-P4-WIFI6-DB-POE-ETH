/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */
#include "MusicPlayer.hpp"
#include "brookesia/service_helper/media/audio.hpp"
#include "brookesia/service_manager/service/manager.hpp"
#include "brookesia/service_helper/system/storage.hpp"
#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <optional>
#include <strings.h>
#include <sys/stat.h>
#include <utility>
#include "esp_log.h"
#include "esp_spiffs.h"
#include "src/lv_demo_music_main.h"

LV_IMAGE_DECLARE(img_app_musicplayer);
namespace esp_brookesia::apps {
static const char *TAG = "MusicPlayer";
static constexpr const char *MUSIC_DIRECTORY = "/spiffs/music";
static constexpr uint32_t PLAYBACK_START_TIMEOUT_MS = 10000;
using AudioPlaybackHelper = service::helper::AudioPlayback;
using AudioPlayState = service::helper::Audio::PlayState;

static int read_saved_media_volume()
{
    // Match the namespace, key, default and range used by Settings/SoundPage.
    constexpr int default_volume = 70;
    auto result = service::helper::Storage::get_key_value<int32_t>("storage", "media_volume", 1000);
    return result ? std::clamp(static_cast<int>(result.value()), 0, 95) : default_volume;
}

class MusicPlayer::PlaybackStateMonitor {
public:
    using EventMonitor = AudioPlaybackHelper::EventMonitor<AudioPlaybackHelper::EventId::PlayStateChanged>;

    ~PlaybackStateMonitor()
    {
        stop();
    }

    bool start()
    {
        auto &manager = service::ServiceManager::get_instance();
        auto binding = manager.bind(AudioPlaybackHelper::get_name().data());
        if(!binding.is_valid()) return false;
        _binding = std::move(binding);
        if(_events.start()) return true;
        _binding.release();
        return false;
    }

    void stop()
    {
        _events.stop();
        if(_binding.is_valid()) _binding.release();
    }

    void clear()
    {
        _events.clear();
    }

    std::optional<AudioPlayState> get_last_state() const
    {
        const auto items = _events.get_last();
        if(!items || items->size() != 1) return std::nullopt;
        const auto *value = std::get_if<std::string>(&items->front());
        if(!value) return std::nullopt;
        if(*value == "Idle") return AudioPlayState::Idle;
        if(*value == "Playing") return AudioPlayState::Playing;
        if(*value == "Paused") return AudioPlayState::Paused;
        return std::nullopt;
    }

    bool has_playing_event() const
    {
        return _events.has({std::string("Playing")});
    }

private:
    service::ServiceBinding _binding;
    EventMonitor _events;
};

MusicPlayer *MusicPlayer::_instance = nullptr;
MusicPlayer *MusicPlayer::requestInstance(bool status_bar, bool navigation_bar) {
    if(!_instance) _instance = new MusicPlayer(status_bar, navigation_bar);
    return _instance;
}

MusicPlayer::MusicPlayer(bool status_bar, bool navigation_bar):
    App("MusicPlayer", &img_app_musicplayer, true, status_bar, navigation_bar) {

}

MusicPlayer::~MusicPlayer() {
    (void)close();
}

bool MusicPlayer::init() {
    return true;
}

bool MusicPlayer::deinit() {
    return close();
}

bool MusicPlayer::back() {
    return notifyCoreClosed();
}

void MusicPlayer::show_message(const char *message, esp_err_t error)
{
    ESP_LOGW(TAG, "%s: %s", message, esp_err_to_name(error));
    if(!_message) {
        _message = lv_label_create(lv_screen_active());
        lv_obj_set_width(_message, lv_pct(90));
        lv_obj_set_style_text_align(_message, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(_message);
    }
    if(error == ESP_OK) lv_label_set_text(_message, message);
    else lv_label_set_text_fmt(_message, "%s\n%s", message, esp_err_to_name(error));
}

bool MusicPlayer::run()
{
    if(!close()) return false;
    if(!esp_spiffs_mounted("storage")) {
        esp_vfs_spiffs_conf_t cfg = {};
        cfg.base_path = "/spiffs";
        cfg.partition_label = "storage";
        cfg.max_files = 5;
        cfg.format_if_mount_failed = false;
        esp_err_t ret = esp_vfs_spiffs_register(&cfg);
        if(ret != ESP_OK) {
            show_message("Music storage unavailable", ret);
            return true;
        }
        _spiffs_owned = true;
    }
    DIR *dir = opendir(MUSIC_DIRECTORY);
    if(dir) {
        while(auto *entry = readdir(dir)) {
            const char *extension = strrchr(entry->d_name, '.');
            if(!extension || strcasecmp(extension, ".mp3") != 0) continue;
            if(strchr(entry->d_name, '/')) continue;
            std::string name(entry->d_name);
            std::string path = std::string(MUSIC_DIRECTORY) + "/" + name;
            struct stat info = {};
            if(stat(path.c_str(), &info) != 0 || !S_ISREG(info.st_mode)) continue;
            _tracks.insert(std::lower_bound(_tracks.begin(), _tracks.end(), name), name);
            if(_tracks.size() > APP_MUSIC_MAX_TRACKS) _tracks.pop_back();
        }
        closedir(dir);
    }
    if(_tracks.empty()) {
        show_message("No MP3 files in /spiffs/music");
        return true;
    }
    auto &service_manager = service::ServiceManager::get_instance();
    if(!service_manager.is_initialized() && !service_manager.init()) {
        show_message("Audio service initialization failed", ESP_FAIL);
        (void)cleanup_player();
        return true;
    }
    if(!service_manager.is_running() && !service_manager.start()) {
        show_message("Audio service startup failed", ESP_FAIL);
        (void)cleanup_player();
        return true;
    }
    _state_monitor = std::make_unique<PlaybackStateMonitor>();
    if(!_state_monitor->start()) {
        show_message("Audio playback service unavailable", ESP_FAIL);
        (void)cleanup_player();
        return true;
    }
    const auto volume_result = AudioPlaybackHelper::call_function_sync(
                                   AudioPlaybackHelper::FunctionId::SetVolume,
                                   static_cast<double>(read_saved_media_volume())
                               );
    if(!volume_result) {
        ESP_LOGE(TAG, "Set playback volume: %s", volume_result.error().c_str());
        show_message("Audio volume initialization failed", ESP_FAIL);
        (void)cleanup_player();
        return true;
    }
    const app_music_backend_t backend = {
        this, static_cast<uint32_t>(_tracks.size()), title_cb, play_cb, pause_cb, resume_cb, status_cb
    };
    app_music_create(lv_screen_active(), &backend);
    _ui_active = true;
    return true;
}

const char *MusicPlayer::title_cb(void *context, uint32_t id)
{
    auto *self = static_cast<MusicPlayer *>(context);
    return id < self->_tracks.size() ? self->_tracks[id].c_str() : nullptr;
}

bool MusicPlayer::play_cb(void *context, uint32_t id)
{
    auto *self = static_cast<MusicPlayer *>(context);
    if(!self->_state_monitor || id >= self->_tracks.size()) return false;
    self->_state_monitor->clear();
    const std::string url = std::string("file://") + MUSIC_DIRECTORY + "/" + self->_tracks[id];
    self->_play_pending = true;
    self->_play_request_ms = lv_tick_get();
    self->_playback_started = false;
    self->_is_counting = false;
    self->_elapsed_ms = 0;
    auto result = AudioPlaybackHelper::call_function_sync(AudioPlaybackHelper::FunctionId::Play, url);
    if(!result) {
        ESP_LOGE(TAG, "Play %s: %s", url.c_str(), result.error().c_str());
        self->_play_pending = false;
        self->_loaded_track = UINT32_MAX;
        return false;
    }
    self->_loaded_track = id;
    return true;
}

bool MusicPlayer::pause_cb(void *context)
{
    auto *self = static_cast<MusicPlayer *>(context);
    if(!self->_state_monitor || self->_loaded_track == UINT32_MAX) return true;
    auto result = AudioPlaybackHelper::call_function_sync(AudioPlaybackHelper::FunctionId::Pause);
    if(!result) {
        ESP_LOGE(TAG, "Pause: %s", result.error().c_str());
        return false;
    }
    if(self->_is_counting) {
        self->_elapsed_ms += lv_tick_get() - self->_play_started_ms;
        self->_is_counting = false;
    }
    return true;
}

bool MusicPlayer::resume_cb(void *context, uint32_t id)
{
    auto *self = static_cast<MusicPlayer *>(context);
    if(!self->_state_monitor || id >= self->_tracks.size()) return false;
    if(self->_loaded_track == id) {
        if(self->_play_pending) return true;
        const auto state = self->_state_monitor->get_last_state();
        if(state == AudioPlayState::Playing) return true;
        if(state == AudioPlayState::Paused) {
            auto result = AudioPlaybackHelper::call_function_sync(AudioPlaybackHelper::FunctionId::Resume);
            if(!result) {
                ESP_LOGE(TAG, "Resume: %s", result.error().c_str());
                return false;
            }
            self->_play_pending = true;
            self->_play_request_ms = lv_tick_get();
            return true;
        }
    }
    return play_cb(context, id);
}

app_music_state_t MusicPlayer::status_cb(void *context, uint32_t *position, uint32_t *duration)
{
    auto *self = static_cast<MusicPlayer *>(context);
    *position = *duration = 0;
    if(!self->_state_monitor || self->_loaded_track == UINT32_MAX) return APP_MUSIC_STOPPED;

    const uint32_t now = lv_tick_get();
    const auto state = self->_state_monitor->get_last_state();
    if(self->_play_pending && state != AudioPlayState::Playing) {
        if(now - self->_play_request_ms < PLAYBACK_START_TIMEOUT_MS) return APP_MUSIC_PLAYING;
        self->_play_pending = false;
        self->_loaded_track = UINT32_MAX;
        return APP_MUSIC_ERROR;
    }
    if(state == AudioPlayState::Playing) {
        self->_play_pending = false;
        self->_playback_started = true;
        if(!self->_is_counting) {
            self->_play_started_ms = now;
            self->_is_counting = true;
        }
        *position = (self->_elapsed_ms + now - self->_play_started_ms) / 1000;
        return APP_MUSIC_PLAYING;
    }
    if(state == AudioPlayState::Paused) {
        self->_playback_started = true;
        if(self->_is_counting) {
            self->_elapsed_ms += now - self->_play_started_ms;
            self->_is_counting = false;
        }
        *position = self->_elapsed_ms / 1000;
        return APP_MUSIC_PAUSED;
    }
    if(state == AudioPlayState::Idle) {
        if(self->_is_counting) {
            self->_elapsed_ms += now - self->_play_started_ms;
            self->_is_counting = false;
        }
        *position = self->_elapsed_ms / 1000;
        self->_playback_started = self->_playback_started || self->_state_monitor->has_playing_event();
        return self->_playback_started ? APP_MUSIC_FINISHED : APP_MUSIC_STOPPED;
    }
    return self->_play_pending ? APP_MUSIC_PLAYING : APP_MUSIC_ERROR;
}

esp_err_t MusicPlayer::cleanup_player()
{
    if(_loaded_track != UINT32_MAX && _state_monitor) {
        auto result = AudioPlaybackHelper::call_function_sync(AudioPlaybackHelper::FunctionId::Stop);
        if(!result) ESP_LOGW(TAG, "Stop playback: %s", result.error().c_str());
    }
    _loaded_track = UINT32_MAX;
    _play_pending = false;
    _playback_started = false;
    _is_counting = false;
    _elapsed_ms = 0;
    if(_state_monitor) {
        _state_monitor->stop();
        _state_monitor.reset();
    }
    if(_spiffs_owned) {
        esp_err_t ret = esp_vfs_spiffs_unregister("storage");
        if(ret != ESP_OK) return ret;
        _spiffs_owned = false;
    }
    return ESP_OK;
}

bool MusicPlayer::close()
{
    const esp_err_t ret = cleanup_player();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Cleanup: %s", esp_err_to_name(ret));
        return false;
    }
    if(_ui_active) app_music_close();
    _ui_active = false;
    if(_message) lv_obj_delete(_message);
    _message = nullptr;
    _tracks.clear();
    return true;
}

bool MusicPlayer::pause()
{
    if(_ui_active) app_music_pause();
    return true;
}

bool MusicPlayer::resume() { return true; }
} // namespace esp_brookesia::apps
