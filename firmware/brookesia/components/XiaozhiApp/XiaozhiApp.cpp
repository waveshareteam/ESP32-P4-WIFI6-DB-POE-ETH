/*
 * SPDX-FileCopyrightText: 2023-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "XiaozhiApp.hpp"
#include "XiaozhiActivationClient.hpp"
#include "XiaozhiActivationPrompt.hpp"
#include "XiaozhiLocalization.hpp"
#include "XiaozhiUi.hpp"

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>
#include <vector>

#include "esp_spiffs.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_xiaozhi_info.h"
#include "sdkconfig.h"
#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:XiaozhiApp"
#include "esp_lib_utils.h"

LV_IMG_DECLARE(img_app_xiaozhi);

namespace esp_brookesia::apps {

namespace {

constexpr uint32_t RETRY_INTERVAL_MS = 5000;
constexpr uint32_t TRANSPORT_RECONNECT_FALLBACK_MS = 10000;
constexpr uint32_t ACTIVATION_RETRY_INTERVAL_MS = 3000;
constexpr uint32_t ACTIVATION_RETRY_MAX_MS = 10000;
constexpr uint32_t CHAT_SAMPLE_RATE = 16000;
constexpr uint32_t CHAT_OUTPUT_SAMPLE_RATE = 24000;
constexpr uint32_t CHAT_FRAME_DURATION_MS = 60;
constexpr uint32_t PLAYBACK_DRAIN_MS = 200;
constexpr uint32_t UPLINK_FIRST_PACKET_TIMEOUT_MS = 3000;
constexpr uint32_t CONTROL_EVENT_POST_TIMEOUT_MS = 100;
constexpr uint32_t LIFECYCLE_WAIT_MS = 10000;
constexpr size_t CONTROL_QUEUE_LENGTH = 24;

bool netifHasIpv4(const char *if_key)
{
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey(if_key);
    if (!netif) {
        return false;
    }
    esp_netif_ip_info_t info = {};
    return esp_netif_get_ip_info(netif, &info) == ESP_OK && info.ip.addr != 0;
}

void copyString(char *destination, size_t capacity, const char *source)
{
    if (destination && capacity > 0) {
        snprintf(destination, capacity, "%s", source ? source : "");
    }
}

uint32_t activationRetryDelay(int timeout_ms)
{
    if (timeout_ms <= 0) {
        return ACTIVATION_RETRY_INTERVAL_MS;
    }
    uint32_t delay = static_cast<uint32_t>(timeout_ms);
    if (delay < 1000) {
        delay = 1000;
    }
    return delay > ACTIVATION_RETRY_MAX_MS ? ACTIVATION_RETRY_MAX_MS : delay;
}

class ScopedSemaphoreLock {
public:
    ScopedSemaphoreLock(SemaphoreHandle_t semaphore, TickType_t timeout):
        _semaphore(semaphore)
    {
        _locked =
            _semaphore && xSemaphoreTake(_semaphore, timeout) == pdTRUE;
    }

    ~ScopedSemaphoreLock()
    {
        if (_locked) {
            xSemaphoreGive(_semaphore);
        }
    }

    explicit operator bool() const
    {
        return _locked;
    }

private:
    SemaphoreHandle_t _semaphore = nullptr;
    bool _locked = false;
};

constexpr uint32_t LIFECYCLE_CLOSED_BIT = 1U << 31;
constexpr uint32_t LIFECYCLE_REF_MASK = LIFECYCLE_CLOSED_BIT - 1;

class ScopedLifecycleAdmission {
public:
    explicit ScopedLifecycleAdmission(std::atomic<uint32_t> &state):
        _state(state)
    {
        uint32_t current = _state.load();
        while ((current & LIFECYCLE_CLOSED_BIT) == 0 &&
                (current & LIFECYCLE_REF_MASK) != LIFECYCLE_REF_MASK) {
            if (_state.compare_exchange_weak(current, current + 1)) {
                _acquired = true;
                break;
            }
        }
    }

    ~ScopedLifecycleAdmission()
    {
        if (_acquired) {
            _state.fetch_sub(1);
        }
    }

    explicit operator bool() const
    {
        return _acquired;
    }

private:
    std::atomic<uint32_t> &_state;
    bool _acquired = false;
};

class ScopedOptionalCounter {
public:
    explicit ScopedOptionalCounter(std::atomic<uint32_t> *counter):
        _counter(counter)
    {
        if (_counter) {
            _counter->fetch_add(1);
        }
    }

    ~ScopedOptionalCounter()
    {
        if (_counter) {
            _counter->fetch_sub(1);
        }
    }

private:
    std::atomic<uint32_t> *_counter = nullptr;
};

class ScopedXiaozhiInfo {
public:
    ScopedXiaozhiInfo() = default;

    ~ScopedXiaozhiInfo()
    {
        (void)esp_xiaozhi_chat_free_info(&value);
    }

    esp_xiaozhi_chat_info_t value = {};

    ScopedXiaozhiInfo(const ScopedXiaozhiInfo &) = delete;
    ScopedXiaozhiInfo &operator=(const ScopedXiaozhiInfo &) = delete;
};

} // namespace

XiaozhiApp *XiaozhiApp::_instance = nullptr;

XiaozhiApp *XiaozhiApp::requestInstance(bool use_status_bar, bool use_navigation_bar)
{
    if (!_instance) {
        _instance = new XiaozhiApp(use_status_bar, use_navigation_bar);
    }
    return _instance;
}

XiaozhiApp::XiaozhiApp(bool use_status_bar, bool use_navigation_bar):
    App("AIChats", &img_app_xiaozhi, true, use_status_bar, use_navigation_bar)
{
    _lifecycle_mutex = xSemaphoreCreateMutex();
}

XiaozhiApp::~XiaozhiApp()
{
    _destroying.store(true);
    while (!deinit()) {
        ESP_UTILS_LOGW("Waiting to deinitialize Xiaozhi safely");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    _lifecycle_refs.fetch_or(LIFECYCLE_CLOSED_BIT);
    while ((_lifecycle_refs.load() & LIFECYCLE_REF_MASK) != 0) {
        vTaskDelay(1);
    }
    if (_lifecycle_mutex) {
        vSemaphoreDelete(_lifecycle_mutex);
        _lifecycle_mutex = nullptr;
    }
    _instance = nullptr;
}

bool XiaozhiApp::run()
{
    _ready_prompt_played.store(false);
    createUi();
    _visible.store(true);
    updateNetworkState();
    ControlEvent event = {};
    event.type = ControlEventType::AppVisible;
    postControlEvent(event);
    return true;
}

bool XiaozhiApp::back()
{
    return hideApp() && notifyCoreClosed();
}

bool XiaozhiApp::hideApp()
{
    _visible.store(false);
    if (_activation_prompt) {
        _activation_prompt->cancel();
    }
    if (_activation_client) {
        _activation_client->cancel();
    }
    ControlEvent event = {};
    event.type = ControlEventType::AppHidden;
    event.flag = true;
    return postControlEvent(event, true);
}

bool XiaozhiApp::close()
{
    if (!hideApp()) {
        return false;
    }
    destroyUi();
    return true;
}

bool XiaozhiApp::init()
{
    ScopedLifecycleAdmission lifecycle_user(_lifecycle_refs);
    if (!lifecycle_user) {
        return false;
    }
    bool deinit_was_complete = _deinit_complete.load();
    if (_destroying.load() || _deinit_in_progress.load()) {
        return false;
    }
    ScopedSemaphoreLock lifecycle_lock(
        _lifecycle_mutex, pdMS_TO_TICKS(LIFECYCLE_WAIT_MS)
    );
    if (!lifecycle_lock) {
        return false;
    }
    if (_destroying.load() || _deinit_in_progress.load() ||
            (!deinit_was_complete && _deinit_complete.load())) {
        return false;
    }
    if (_control_running.load()) {
        return true;
    }

    _deinit_complete.store(false);
    _shutdown.store(false);
    _activation_phase = ActivationPhase::CheckRequired;
    _control_event_lost.store(false);
    _state_mutex = _state_mutex ? _state_mutex : xSemaphoreCreateMutex();
    _audio_tx_mutex = _audio_tx_mutex ? _audio_tx_mutex : xSemaphoreCreateMutex();
    _control_ack = _control_ack ? _control_ack : xSemaphoreCreateBinary();
    _control_exited = _control_exited ? _control_exited : xSemaphoreCreateBinary();
    _control_queue = _control_queue ? _control_queue :
                     xQueueCreate(CONTROL_QUEUE_LENGTH, sizeof(ControlEvent));
    if (!_state_mutex || !_lifecycle_mutex || !_audio_tx_mutex || !_control_ack ||
            !_control_exited || !_control_queue) {
        setError(ESP_ERR_NO_MEM, "Create Xiaozhi synchronization objects");
        return false;
    }

    _activation_client = _activation_client ? _activation_client :
                         new (std::nothrow) XiaozhiActivationClient();
    _activation_prompt = _activation_prompt ? _activation_prompt :
                         new (std::nothrow) XiaozhiActivationPrompt();
    _ui = _ui ? _ui : new (std::nothrow) XiaozhiUi();
    if (!_activation_client || !_activation_prompt || !_ui) {
        setError(ESP_ERR_NO_MEM, "Create Xiaozhi helpers");
        return false;
    }
    // This board pack has no fs_spiffs device. Share the existing application
    // storage mount with MusicPlayer; never format or unmount it here.
    if (!esp_spiffs_mounted("storage")) {
        esp_vfs_spiffs_conf_t storage_config = {};
        storage_config.base_path = "/spiffs";
        storage_config.partition_label = "storage";
        storage_config.max_files = 5;
        storage_config.format_if_mount_failed = false;
        esp_err_t storage_ret = esp_vfs_spiffs_register(&storage_config);
        if (storage_ret != ESP_OK) {
            ESP_UTILS_LOGW("Xiaozhi assets unavailable: %s", esp_err_to_name(storage_ret));
        }
    }
    (void)_ui->preload();

    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        setError(ret, "Initialize network interfaces");
        return false;
    }
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        setError(ret, "Create default event loop");
        return false;
    }
    if (!_ip_event_instance) {
        ret = esp_event_handler_instance_register(
                  IP_EVENT, ESP_EVENT_ANY_ID, ipEventHandler, this, &_ip_event_instance
              );
        if (ret != ESP_OK) {
            setError(ret, "Register network event handler");
            return false;
        }
    }

    _control_running.store(true);
    if (xTaskCreatePinnedToCore(
            controlTaskEntry,
            "xiaozhi_control",
            CONTROL_TASK_STACK_SIZE,
            this,
            5,
            &_control_task,
            0
        ) != pdPASS) {
        _control_running.store(false);
        _control_task = nullptr;
        setError(ESP_ERR_NO_MEM, "Create Xiaozhi control task");
        return false;
    }
    updateNetworkState();
    return true;
}

bool XiaozhiApp::deinit()
{
    ScopedLifecycleAdmission lifecycle_user(_lifecycle_refs);
    if (!lifecycle_user) {
        return false;
    }
    if (_deinit_complete.load()) {
        return true;
    }
    bool expected = false;
    if (!_deinit_in_progress.compare_exchange_strong(expected, true)) {
        TickType_t started = xTaskGetTickCount();
        while (_deinit_in_progress.load() &&
                xTaskGetTickCount() - started <
                    pdMS_TO_TICKS(LIFECYCLE_WAIT_MS)) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        return _deinit_complete.load();
    }
    auto finish_deinit = [this](bool complete) {
        _deinit_complete.store(complete);
        _deinit_in_progress.store(false);
        return complete;
    };

    if (!_lifecycle_mutex && !_control_running.load()) {
        _shutdown.store(true);
        return finish_deinit(true);
    }

    ScopedSemaphoreLock lifecycle_lock(
        _lifecycle_mutex, pdMS_TO_TICKS(LIFECYCLE_WAIT_MS)
    );
    if (!lifecycle_lock) {
        ESP_UTILS_LOGE("Timed out waiting for Xiaozhi lifecycle lock");
        return finish_deinit(false);
    }

    _visible.store(false);
    bool was_shutdown = _shutdown.exchange(true);
    if (_activation_prompt) {
        _activation_prompt->cancel();
    }
    if (_activation_client) {
        _activation_client->cancel();
    }
    bool control_stopped = true;
    if (_control_running.load()) {
        control_stopped = _control_queue && _control_exited;
        if (control_stopped && !was_shutdown) {
            ControlEvent event = {};
            event.type = ControlEventType::Shutdown;
            control_stopped =
                xQueueSendToFront(
                    _control_queue,
                    &event,
                    pdMS_TO_TICKS(LIFECYCLE_WAIT_MS)
                ) == pdTRUE;
            if (!control_stopped) {
                _shutdown.store(false);
            }
        }
        if (control_stopped) {
            control_stopped =
                xSemaphoreTake(
                    _control_exited, pdMS_TO_TICKS(LIFECYCLE_WAIT_MS)
                ) == pdTRUE;
        }
    }
    if (!control_stopped) {
        ESP_UTILS_LOGE("Xiaozhi control task did not stop cleanly");
        return finish_deinit(false);
    }
    if (_ip_event_instance) {
        esp_event_handler_instance_unregister(
            IP_EVENT, ESP_EVENT_ANY_ID, _ip_event_instance
        );
        _ip_event_instance = nullptr;
    }

    TickType_t post_wait_started = xTaskGetTickCount();
    while (_async_post_users.load() != 0 &&
            xTaskGetTickCount() - post_wait_started <
                pdMS_TO_TICKS(LIFECYCLE_WAIT_MS)) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (_async_post_users.load() != 0) {
        ESP_UTILS_LOGE("Xiaozhi callbacks still have active producers");
        return finish_deinit(false);
    }

    // Let the task finish returning from the exit semaphore give before the
    // semaphore and queues are deleted.
    vTaskDelay(1);

    destroyUi();
    delete _ui;
    _ui = nullptr;
    delete _activation_prompt;
    _activation_prompt = nullptr;
    delete _activation_client;
    _activation_client = nullptr;

    if (_control_queue) {
        vQueueDelete(_control_queue);
        _control_queue = nullptr;
    }
    if (_control_exited) {
        vSemaphoreDelete(_control_exited);
        _control_exited = nullptr;
    }
    if (_state_mutex) {
        vSemaphoreDelete(_state_mutex);
        _state_mutex = nullptr;
    }
    return finish_deinit(true);
}

bool XiaozhiApp::pause()
{
    if (!hideApp()) {
        return false;
    }
    if (_ui_timer) {
        lv_timer_pause(_ui_timer);
    }
    return true;
}

bool XiaozhiApp::resume()
{
    if (!_page_root) {
        createUi();
    }
    _visible.store(true);
    if (_ui_timer) {
        lv_timer_resume(_ui_timer);
    }
    updateNetworkState();
    ControlEvent event = {};
    event.type = ControlEventType::AppVisible;
    postControlEvent(event);
    return true;
}

void XiaozhiApp::createUi()
{
    destroyUi();
    if (!_ui) {
        return;
    }

    lv_obj_t *screen = lv_screen_active();
    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    _page_root = lv_obj_create(screen);
    lv_obj_remove_style_all(_page_root);
    lv_obj_set_size(_page_root, lv_pct(100), lv_pct(100));
    lv_obj_center(_page_root);
    lv_obj_set_style_bg_color(_page_root, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_page_root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(_page_root, LV_OBJ_FLAG_SCROLLABLE);

    if (!_ui->create(_page_root, uiActionCallback, this, false)) {
        destroyUi();
        return;
    }
    _ui_timer = lv_timer_create(uiTimerCallback, 100, this);
    _ui_dirty.store(true);
    refreshUi();
}

void XiaozhiApp::destroyUi()
{
    if (_ui_timer) {
        lv_timer_delete(_ui_timer);
        _ui_timer = nullptr;
    }
    if (_ui) {
        _ui->destroy();
    }
    if (_page_root) {
        lv_obj_delete(_page_root);
        _page_root = nullptr;
    }
}

void XiaozhiApp::refreshUi()
{
    if (!_ui || !_page_root) {
        return;
    }
    // Consume before taking the snapshot so concurrent updates remain pending.
    if (!_ui_dirty.exchange(false)) {
        return;
    }
    char code[sizeof(_activation_code)] = {};
    char message[sizeof(_activation_message)] = {};
    char role[sizeof(_chat_role)] = {};
    char text[sizeof(_chat_text)] = {};
    char emotion[sizeof(_emotion)] = {};
    if (!_state_mutex ||
            xSemaphoreTake(_state_mutex, 0) != pdTRUE) {
        _ui_dirty.store(true);
        return;
    }
    memcpy(code, _activation_code, sizeof(code));
    memcpy(message, _activation_message, sizeof(message));
    memcpy(role, _chat_role, sizeof(role));
    memcpy(text, _chat_text, sizeof(text));
    memcpy(emotion, _emotion, sizeof(emotion));
    xSemaphoreGive(_state_mutex);

    State state = _state.load();
    _ui->setNetworkReady(_network_ready.load());
    _ui->setStatus(stateText(state));
    _ui->setChatMessage(role, text);
    _ui->setEmotion(emotion[0] ? emotion : "neutral");
    _ui->setActivation(
        code,
        message[0] ? message : xiaozhiUiText(
            XiaozhiUiText::ActivationInstructions
        ),
        state == State::ActivationRequired
    );
}

void XiaozhiApp::setState(State state)
{
    if (_state.exchange(state) != state) {
        _ui_dirty.store(true);
    }
}

void XiaozhiApp::setError(esp_err_t error, const char *source)
{
    ESP_UTILS_LOGE(
        "%s: %s", source ? source : "Xiaozhi", esp_err_to_name(error)
    );
    if (_network_ready.load()) {
        setState(State::Error);
    }
}

void XiaozhiApp::setChatMessage(XiaozhiClient::TextRole role, const char *text)
{
    if (!_state_mutex || xSemaphoreTake(_state_mutex, portMAX_DELAY) != pdTRUE) {
        return;
    }
    copyString(
        _chat_role,
        sizeof(_chat_role),
        role == XiaozhiClient::TextRole::Assistant ? "assistant" : "user"
    );
    copyString(_chat_text, sizeof(_chat_text), text);
    _ui_dirty.store(true);
    xSemaphoreGive(_state_mutex);
}

void XiaozhiApp::setEmotion(const char *emotion)
{
    if (_state_mutex && xSemaphoreTake(_state_mutex, portMAX_DELAY) == pdTRUE) {
        copyString(_emotion, sizeof(_emotion), emotion ? emotion : "neutral");
        _ui_dirty.store(true);
        xSemaphoreGive(_state_mutex);
    }
}

void XiaozhiApp::clearConversationUi()
{
    if (_state_mutex && xSemaphoreTake(_state_mutex, portMAX_DELAY) == pdTRUE) {
        _chat_role[0] = '\0';
        _chat_text[0] = '\0';
        copyString(_emotion, sizeof(_emotion), "neutral");
        _ui_dirty.store(true);
        xSemaphoreGive(_state_mutex);
    }
}

void XiaozhiApp::copyActivation(const char *code, const char *message)
{
    if (_state_mutex && xSemaphoreTake(_state_mutex, portMAX_DELAY) == pdTRUE) {
        copyString(_activation_code, sizeof(_activation_code), code);
        copyString(_activation_message, sizeof(_activation_message), message);
        _ui_dirty.store(true);
        xSemaphoreGive(_state_mutex);
    }
}

void XiaozhiApp::clearActivation()
{
    if (_state_mutex && xSemaphoreTake(_state_mutex, portMAX_DELAY) == pdTRUE) {
        _activation_code[0] = '\0';
        _activation_message[0] = '\0';
        _ui_dirty.store(true);
        xSemaphoreGive(_state_mutex);
    }
}

bool XiaozhiApp::announceActivation(bool force)
{
    if (!_visible.load() || !_activation_prompt ||
            _state.load() != State::ActivationRequired) {
        return false;
    }
    char code[sizeof(_activation_code)] = {};
    if (_state_mutex && xSemaphoreTake(_state_mutex, portMAX_DELAY) == pdTRUE) {
        copyString(code, sizeof(code), _activation_code);
        xSemaphoreGive(_state_mutex);
    }
    return _activation_prompt->start(code, force);
}

void XiaozhiApp::stopActivationPrompt()
{
    if (!_activation_prompt) {
        return;
    }
    _activation_prompt->cancel();
    while (!_activation_prompt->stopAndWait(1000)) {
        ESP_UTILS_LOGW("Waiting for Xiaozhi prompt audio to stop");
    }
}

bool XiaozhiApp::playReadyPromptOnce()
{
    if (_ready_prompt_played.exchange(true)) {
        return true;
    }
    if (!_activation_prompt || !canPrepare()) {
        ESP_UTILS_LOGW("Xiaozhi ready prompt is unavailable");
        return false;
    }

    stopActivationPrompt();
    if (_audio.isReady()) {
        shutdownAudio();
    }

    bool completed = false;
    if (_activation_prompt->playSuccess()) {
        while (!_activation_prompt->waitUntilIdle(100)) {
            if (!canPrepare()) {
                _activation_prompt->cancel();
                return false;
            }
        }
        completed = _activation_prompt->lastPlaybackCompleted();
    }
    if (completed) {
        ESP_UTILS_LOGI("Xiaozhi self-check completed; success prompt played");
    } else {
        ESP_UTILS_LOGW("Xiaozhi self-check completed without success prompt");
    }
    return completed;
}

bool XiaozhiApp::canPrepare() const
{
    return _visible.load() && _network_ready.load() && isNetworkReady() &&
           !_shutdown.load();
}

bool XiaozhiApp::isNetworkReady() const
{
    return netifHasIpv4("WIFI_STA_DEF") || netifHasIpv4("ETH_DEF");
}

void XiaozhiApp::updateNetworkState()
{
    bool ready = isNetworkReady();
    if (!ready) {
        if (_activation_prompt) {
            _activation_prompt->cancel();
        }
        if (_activation_client) {
            _activation_client->cancel();
        }
    }
    ControlEvent event = {};
    event.type = ControlEventType::NetworkChanged;
    event.flag = ready;
    postControlEvent(event);
}

bool XiaozhiApp::postControlEvent(const ControlEvent &event, bool wait_for_ack)
{
    ScopedLifecycleAdmission lifecycle_user(_lifecycle_refs);
    if (!lifecycle_user) {
        return false;
    }
    ScopedOptionalCounter async_post_user(
        wait_for_ack ? nullptr : &_async_post_users
    );
    if (_destroying.load() || _deinit_in_progress.load() ||
            !_control_queue || (!_control_running.load() &&
                            event.type != ControlEventType::Shutdown) ||
            (_shutdown.load() && event.type != ControlEventType::Shutdown)) {
        return false;
    }
    if (!wait_for_ack) {
        bool best_effort =
            event.type == ControlEventType::ChatText ||
            event.type == ControlEventType::Emotion;
        TickType_t wait_ticks =
            best_effort ? 0 : pdMS_TO_TICKS(CONTROL_EVENT_POST_TIMEOUT_MS);
        bool queued = xQueueSend(_control_queue, &event, wait_ticks) == pdTRUE;
        if (!queued) {
            ESP_UTILS_LOGW(
                "Control queue rejected event %u",
                static_cast<unsigned>(event.type)
            );
            if (!best_effort) {
                _control_event_lost.store(true);
            }
        }
        return queued;
    }
    if (!_lifecycle_mutex || !_control_ack) {
        return false;
    }
    if (xSemaphoreTake(
                _lifecycle_mutex, pdMS_TO_TICKS(LIFECYCLE_WAIT_MS)
            ) != pdTRUE) {
        return false;
    }
    if (!_control_queue || !_control_running.load() || _shutdown.load()) {
        xSemaphoreGive(_lifecycle_mutex);
        return false;
    }
    xSemaphoreTake(_control_ack, 0);
    bool ok =
        xQueueSend(
            _control_queue, &event, pdMS_TO_TICKS(LIFECYCLE_WAIT_MS)
        ) == pdTRUE;
    if (ok) {
        ok =
            xSemaphoreTake(
                _control_ack, pdMS_TO_TICKS(LIFECYCLE_WAIT_MS)
            ) == pdTRUE;
    }
    xSemaphoreGive(_lifecycle_mutex);
    return ok;
}

void XiaozhiApp::controlLoop()
{
    bool exit_requested = false;
    while (!exit_requested) {
        ControlEvent event = {};
        if (xQueueReceive(_control_queue, &event, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (event.type == ControlEventType::Shutdown) {
                exit_requested = true;
            } else {
                handleControlEvent(event);
                if (event.type == ControlEventType::AppHidden && _control_ack) {
                    xSemaphoreGive(_control_ack);
                }
            }
        }
        if (!exit_requested) {
            if (_control_event_lost.exchange(false)) {
                ESP_UTILS_LOGE("Recovering after a dropped Xiaozhi control event");
                setAudioModes(false, false);
                closeChannel(true);
                destroyChat();
                if (_network_ready.load()) {
                    setError(ESP_ERR_TIMEOUT, "Xiaozhi control queue");
                    schedulePrepare(RETRY_INTERVAL_MS);
                } else {
                    setState(State::NetworkRequired);
                }
            }
            checkPlaybackDrained();
            prepareIfDue();
            TickType_t uplink_started = _uplink_started_tick.load();
            if (uplink_started != 0 &&
                    _state.load() == State::Listening &&
                    _voice_enabled.load() &&
                    _uplink_ready.load() &&
                    _opus_packets_sent.load() == 0 &&
                    xTaskGetTickCount() - uplink_started >=
                        pdMS_TO_TICKS(UPLINK_FIRST_PACKET_TIMEOUT_MS)) {
                ESP_UTILS_LOGE(
                    "No Xiaozhi Opus uplink packet after listening started"
                );
                _uplink_started_tick.store(0);
                closeChannel(true);
                destroyChat();
                if (_network_ready.load()) {
                    setError(ESP_ERR_TIMEOUT, "Start Xiaozhi audio uplink");
                    schedulePrepare(RETRY_INTERVAL_MS);
                } else {
                    setState(State::NetworkRequired);
                }
            }
        }
    }

    _visible.store(false);
    stopActivationPrompt();
    setAudioModes(false, false);
    closeChannel(true);
    destroyChat();
    shutdownAudio();
    _control_task = nullptr;
    _control_running.store(false);
    if (_control_exited) {
        xSemaphoreGive(_control_exited);
    }
    vTaskDelete(nullptr);
}

void XiaozhiApp::handleControlEvent(const ControlEvent &event)
{
    if ((event.chat_generation != 0 &&
            event.chat_generation != _chat_generation.load()) ||
            (event.turn_generation != 0 &&
             event.turn_generation != _turn_generation.load())) {
        ESP_UTILS_LOGD("Ignoring a stale Xiaozhi control event");
        return;
    }

    switch (event.type) {
    case ControlEventType::AppVisible:
        handleAppVisible();
        break;
    case ControlEventType::AppHidden:
        handleAppHidden(event.flag);
        break;
    case ControlEventType::NetworkChanged:
        // IP events can repeat without changing connectivity. Re-entering the
        // visible state here would interrupt an active listening turn.
        if (_network_ready.exchange(event.flag) == event.flag) {
            break;
        }
        _ui_dirty.store(true);
        if (!event.flag) {
            stopActivationPrompt();
            setState(State::NetworkRequired);
            setAudioModes(false, false);
            closeChannel(false);
            destroyChat();
        } else if (!_chat_started.load() && _visible.load()) {
            if (_activation_phase == ActivationPhase::WaitingForBinding ||
                    _activation_phase == ActivationPhase::BoundPendingSuccess) {
                setState(State::ActivationRequired);
                if (_activation_phase == ActivationPhase::WaitingForBinding) {
                    announceActivation(false);
                }
            } else {
                setState(State::Preparing);
            }
            schedulePrepare(0);
        } else if (_transport_connected) {
            handleAppVisible();
        }
        break;
    case ControlEventType::ActionPressed:
        if (_state.load() == State::ActivationRequired) {
            announceActivation(true);
        } else if (_state.load() == State::Ready) {
            startListening(ListenMode::AutoStop);
        } else if (_state.load() == State::Listening &&
                   _listen_mode == ListenMode::Manual) {
            finishListening();
        } else if ((_state.load() == State::Speaking ||
                    _state.load() == State::Processing) &&
                   _listen_mode == ListenMode::Manual) {
            cancelManualConversation();
        } else if (_state.load() == State::Listening ||
                   _state.load() == State::Speaking ||
                   _state.load() == State::Processing) {
            closeChannel(true);
            setState(State::Ready);
            enterReadyAudioMode();
        }
        break;
    case ControlEventType::AnnounceActivation:
        announceActivation(true);
        break;
    case ControlEventType::WakeWord:
    {
        const State state = _state.load();
        XiaozhiClient *protocol = _protocol.load();
        if (_visible.load() && state == State::Ready) {
            startListening(ListenMode::AutoStop, event.text);
        } else if (_visible.load() && state == State::Speaking &&
                   _channel_state.load() == ChannelState::Open &&
                   protocol) {
            esp_err_t abort_ret = ESP_ERR_INVALID_STATE;
            if (_audio_tx_mutex &&
                    xSemaphoreTake(_audio_tx_mutex, portMAX_DELAY) == pdTRUE) {
                abort_ret = protocol->sendAbortSpeaking(
                    XiaozhiClient::AbortReason::WakeWordDetected
                );
                xSemaphoreGive(_audio_tx_mutex);
            }
            if (abort_ret != ESP_OK) {
                ESP_UTILS_LOGW(
                    "Failed to abort speaking after wake word: 0x%x",
                    static_cast<unsigned int>(abort_ret)
                );
                (void)setAudioModes(true, false);
            } else {
                startListening(ListenMode::AutoStop, event.text);
            }
        } else if (!_visible.load()) {
            setAudioModes(false, false);
        } else {
            ESP_UTILS_LOGD("Wake word ignored in the current conversation state");
            (void)setAudioModes(true, false);
        }
        break;
    }
    case ControlEventType::TransportConnected:
        _transport_connected = true;
        _server_goodbye_generation.store(0);
        if (_chat_started.load() &&
                (_state.load() == State::Connecting ||
                 _state.load() == State::Preparing ||
                 _state.load() == State::Error)) {
            setState(State::Ready);
            if (_visible.load()) {
                (void)playReadyPromptOnce();
                if (canPrepare() && ensureAudioReady()) {
                    enterReadyAudioMode();
                } else if (canPrepare()) {
                    schedulePrepare(RETRY_INTERVAL_MS);
                }
            }
        }
        break;
    case ControlEventType::TransportDisconnected: {
        const uint32_t goodbye_generation =
            _server_goodbye_generation.exchange(0);
        const bool reconnect_now = goodbye_generation == event.chat_generation;
        const bool drain_final_response =
            _tts_stop_pending.load() && _state.load() == State::Speaking;
        _transport_connected = false;
        if (drain_final_response) {
            _uplink_ready.store(false);
            _channel_state.store(ChannelState::Closing);
            _restart_transport_after_playback.store(true);
            ESP_UTILS_LOGI(
                "Transport closed after final TTS; reconnecting after playback drains"
            );
            break;
        }

        _restart_transport_after_playback.store(false);
        resetChannelState();
        setAudioModes(false, false);
        if (reconnect_now) {
            ESP_UTILS_LOGI(
                "Transport closed after server goodbye; reconnecting now"
            );
            destroyChat();
        } else {
            ESP_UTILS_LOGW(
                "Xiaozhi transport disconnected; waiting for auto reconnect"
            );
        }
        if (_network_ready.load()) {
            setState(State::Connecting);
            schedulePrepare(
                reconnect_now ? 0 : TRANSPORT_RECONNECT_FALLBACK_MS
            );
        } else {
            setState(State::NetworkRequired);
        }
        break;
    }
    case ControlEventType::ChannelOpened:
        if (_channel_state.load() == ChannelState::Opening) {
            _channel_state.store(ChannelState::Open);
        }
        break;
    case ControlEventType::ChannelClosed:
        if (_channel_state.load() == ChannelState::Closing) {
            resetChannelState();
            if (_network_ready.load() && _transport_connected && _visible.load()) {
                setState(State::Ready);
                enterReadyAudioMode();
            }
        }
        break;
    case ControlEventType::ServerGoodbye:
        _server_goodbye_generation.store(event.chat_generation);
        closeChannel(false);
        if (_network_ready.load() && _transport_connected) {
            ESP_UTILS_LOGI(
                "Server ended the conversation; wake-word standby restored"
            );
            setState(State::Ready);
            enterReadyAudioMode();
        }
        break;
    case ControlEventType::ChatText:
        if (_channel_state.load() == ChannelState::Open &&
                _listen_mode != ListenMode::None) {
            ESP_UTILS_LOGI(
                "Server text received (role=%u, bytes=%u)",
                static_cast<unsigned int>(event.role),
                static_cast<unsigned int>(strlen(event.text))
            );
            setChatMessage(
                static_cast<XiaozhiClient::TextRole>(event.role), event.text
            );
        }
        break;
    case ControlEventType::Emotion:
        if (_channel_state.load() == ChannelState::Open &&
                _listen_mode != ListenMode::None) {
            setEmotion(event.text);
        }
        break;
    case ControlEventType::TtsStart: {
        State state = _state.load();
        if (_channel_state.load() == ChannelState::Open &&
                _listen_mode != ListenMode::None &&
                (state == State::Listening ||
                 state == State::Processing ||
                 state == State::Speaking)) {
            if (!setAudioModes(_visible.load(), false) && _visible.load()) {
                closeChannel(false);
                shutdownAudio();
                destroyChat();
                setError(ESP_FAIL, "Enable wake-word interruption");
                schedulePrepare(RETRY_INTERVAL_MS);
                break;
            }
            _awaiting_response = false;
            _tts_active = true;
            _tts_stop_pending.store(false);
            _accept_playback.store(true);
            ESP_UTILS_LOGI("Server TTS started");
            setState(State::Speaking);
        }
        break;
    }
    case ControlEventType::TtsStop: {
        if (_channel_state.load() == ChannelState::Open &&
                (_tts_active || _state.load() == State::Speaking)) {
            TickType_t now = xTaskGetTickCount();
            _tts_active = false;
            ESP_UTILS_LOGI("Server TTS stopped; draining playback");
            _tts_stop_tick.store(now);
            _last_audio_packet_tick.store(now);
            _drain_event_posted.store(false);
            _tts_stop_pending.store(true);
        }
        break;
    }
    case ControlEventType::PlaybackDrained:
        handlePlaybackDrained();
        break;
    case ControlEventType::ChatError:
        closeChannel(true);
        destroyChat();
        if (_network_ready.load()) {
            setError(event.error, event.detail);
            schedulePrepare(RETRY_INTERVAL_MS);
        } else {
            setState(State::NetworkRequired);
        }
        break;
    case ControlEventType::AudioError:
        closeChannel(true);
        shutdownAudio();
        if (_network_ready.load()) {
            setError(event.error, event.detail);
            bool recovered = false;
            if (_visible.load() && _transport_connected &&
                    _chat_started.load() && ensureAudioReady()) {
                setState(State::Ready);
                recovered = enterReadyAudioMode();
            }
            if (!recovered) {
                schedulePrepare(RETRY_INTERVAL_MS);
            }
        } else {
            setState(State::NetworkRequired);
        }
        break;
    case ControlEventType::Shutdown:
        break;
    }
}

void XiaozhiApp::handleAppVisible()
{
    _visible.store(true);
    if (!_network_ready.load() || !isNetworkReady()) {
        setState(State::NetworkRequired);
    } else if (_activation_phase == ActivationPhase::WaitingForBinding ||
               _activation_phase == ActivationPhase::BoundPendingSuccess) {
        setState(State::ActivationRequired);
        if (_activation_phase == ActivationPhase::WaitingForBinding) {
            announceActivation(false);
        }
        schedulePrepare(0);
    } else if (!_chat_started.load()) {
        setState(State::Preparing);
        schedulePrepare(0);
    } else if (!_transport_connected) {
        setState(State::Connecting);
    } else if (_tts_active || _tts_stop_pending.load()) {
        setState(State::Speaking);
    } else if (_awaiting_response) {
        setState(State::Processing);
    } else {
        setState(State::Ready);
        (void)playReadyPromptOnce();
        if (!ensureAudioReady()) {
            schedulePrepare(RETRY_INTERVAL_MS);
        } else {
            enterReadyAudioMode();
        }
    }
}

void XiaozhiApp::handleAppHidden(bool release_audio)
{
    _visible.store(false);
    stopActivationPrompt();
    setAudioModes(false, false);
    if (release_audio) {
        closeChannel(true);
        shutdownAudio();
    } else if (_listen_mode != ListenMode::None && !_tts_active) {
        stopUplink(true);
    }
}

void XiaozhiApp::prepareIfDue()
{
    if (!canPrepare() ||
            !tickReached(xTaskGetTickCount(), _next_prepare_tick)) {
        return;
    }

    if (_chat_started.load()) {
        if (!_transport_connected && _state.load() == State::Connecting) {
            ESP_UTILS_LOGW(
                "Xiaozhi auto reconnect timed out; restarting transport"
            );
            destroyChat();
            if (canPrepare()) {
                prepareChat();
            }
            return;
        }
        if (_transport_connected && _visible.load() &&
                _state.load() == State::Error &&
                !_audio.isReady()) {
            if (ensureAudioReady()) {
                setState(State::Ready);
                enterReadyAudioMode();
            } else {
                schedulePrepare(RETRY_INTERVAL_MS);
            }
        }
        return;
    }
    prepareChat();
}

void XiaozhiApp::schedulePrepare(uint32_t delay_ms)
{
    _next_prepare_tick = xTaskGetTickCount() + pdMS_TO_TICKS(delay_ms);
}

bool XiaozhiApp::prepareChat()
{
    if (!canPrepare()) {
        return false;
    }
    if (!_activation_client) {
        setError(ESP_ERR_INVALID_STATE, "Initialize Xiaozhi activation");
        schedulePrepare(RETRY_INTERVAL_MS);
        return false;
    }

    setState(State::Preparing);
    const bool was_waiting =
        _activation_phase == ActivationPhase::WaitingForBinding ||
        _activation_phase == ActivationPhase::BoundPendingSuccess;
    ScopedXiaozhiInfo service;
    esp_err_t ret = esp_xiaozhi_chat_get_info(&service.value);
    if (!canPrepare()) {
        return false;
    }
    if (ret != ESP_OK) {
        if (was_waiting) {
            ESP_UTILS_LOGW(
                "Activation status check failed: %s",
                esp_err_to_name(ret)
            );
            setState(State::ActivationRequired);
        } else {
            setError(ret, "Get Xiaozhi service information");
        }
        schedulePrepare(RETRY_INTERVAL_MS);
        return false;
    }

    if (service.value.has_activation_code ||
            service.value.has_activation_challenge) {
        copyActivation(
            service.value.activation_code,
            service.value.activation_message
        );
        _activation_phase = ActivationPhase::WaitingForBinding;
        setState(State::ActivationRequired);
        announceActivation(false);

        const uint32_t delay_ms =
            activationRetryDelay(service.value.activation_timeout_ms);
        if (!service.value.has_activation_challenge ||
                !service.value.activation_challenge ||
                service.value.activation_challenge[0] == 0) {
            schedulePrepare(delay_ms);
            return false;
        }

        XiaozhiActivationClient::Info activation = {};
        activation.state =
            XiaozhiActivationClient::BindingState::ActivationRequired;
        activation.challenge = service.value.activation_challenge;
        int http_status = 0;
        ret = _activation_client->activate(activation, http_status);
        if (!canPrepare()) {
            return false;
        }
        if (ret != ESP_OK && ret != ESP_ERR_TIMEOUT) {
            ESP_UTILS_LOGW(
                "Activation request failed: %s (HTTP %d)",
                esp_err_to_name(ret),
                http_status
            );
        } else if (ret == ESP_OK) {
            _activation_phase = ActivationPhase::BoundPendingSuccess;
            schedulePrepare(0);
            return false;
        }
        schedulePrepare(delay_ms);
        return false;
    }

    if (was_waiting) {
        stopActivationPrompt();
        if (!canPrepare()) {
            return false;
        }
    }
    clearActivation();
    _activation_phase = ActivationPhase::Complete;

    bool use_mqtt = false;
    bool use_websocket = false;
#if CONFIG_XIAOZHI_APP_TRANSPORT_MQTT
    use_mqtt = service.value.has_mqtt_config;
#elif CONFIG_XIAOZHI_APP_TRANSPORT_AUTO
    use_mqtt = service.value.has_mqtt_config;
    use_websocket = service.value.has_websocket_config;
#else
    use_websocket = service.value.has_websocket_config;
#endif
    if (!use_mqtt && !use_websocket) {
#if CONFIG_XIAOZHI_APP_TRANSPORT_MQTT
        setError(ESP_ERR_NOT_SUPPORTED, "Get Xiaozhi MQTT configuration");
#elif CONFIG_XIAOZHI_APP_TRANSPORT_AUTO
        setError(ESP_ERR_NOT_SUPPORTED, "Get Xiaozhi transport configuration");
#else
        setError(ESP_ERR_NOT_SUPPORTED, "Get Xiaozhi WebSocket configuration");
#endif
        schedulePrepare(RETRY_INTERVAL_MS);
        return false;
    }
    if (!canPrepare()) {
        return false;
    }

    const uint32_t client_generation = _chat_generation.fetch_add(1) + 1;
    auto post_simple = [this, client_generation](ControlEventType type) {
        ControlEvent event = {};
        event.type = type;
        event.chat_generation = client_generation;
        (void)postControlEvent(event);
    };

    XiaozhiClient::Callbacks callbacks = {};
    callbacks.onConnected = [post_simple]() {
        post_simple(ControlEventType::TransportConnected);
    };
    callbacks.onDisconnected = [this, post_simple]() {
        if (_chat_started.load()) {
            post_simple(ControlEventType::TransportDisconnected);
        }
    };
    callbacks.onAudioChannelOpened = [post_simple]() {
        post_simple(ControlEventType::ChannelOpened);
    };
    callbacks.onAudioChannelClosed = [post_simple]() {
        post_simple(ControlEventType::ChannelClosed);
    };
    callbacks.onServerGoodbye = [this, post_simple, client_generation]() {
        if (_chat_generation.load() == client_generation) {
            uint32_t marked_generation =
                _server_goodbye_generation.load();
            while (marked_generation < client_generation &&
                    !_server_goodbye_generation.compare_exchange_weak(
                        marked_generation, client_generation
                    )) {
            }
        }
        post_simple(ControlEventType::ServerGoodbye);
    };
    callbacks.onText = [this, client_generation](
        XiaozhiClient::TextRole role,
        const std::string &text
    ) {
        ControlEvent event = {};
        event.type = ControlEventType::ChatText;
        event.role = static_cast<uint8_t>(role);
        event.chat_generation = client_generation;
        event.turn_generation = _turn_generation.load();
        copyString(event.text, sizeof(event.text), text.c_str());
        (void)postControlEvent(event);
    };
    callbacks.onEmotion = [this, client_generation](const std::string &emotion) {
        ControlEvent event = {};
        event.type = ControlEventType::Emotion;
        event.chat_generation = client_generation;
        event.turn_generation = _turn_generation.load();
        copyString(event.text, sizeof(event.text), emotion.c_str());
        (void)postControlEvent(event);
    };
    callbacks.onTts = [this, client_generation](
        XiaozhiClient::TtsState state,
        const std::string &text
    ) {
        if (_chat_generation.load() != client_generation) {
            return;
        }
        (void)text;
        if (state == XiaozhiClient::TtsState::SentenceStart) {
            return;
        }
        ControlEvent event = {};
        event.type = state == XiaozhiClient::TtsState::Start ?
                     ControlEventType::TtsStart : ControlEventType::TtsStop;
        event.chat_generation = client_generation;
        event.turn_generation = _turn_generation.load();
        if (state == XiaozhiClient::TtsState::Start &&
                _channel_state.load() == ChannelState::Open &&
                _chat_started.load() && !_shutdown.load()) {
            _accept_playback.store(true);
        }
        (void)postControlEvent(event);
    };
    callbacks.onAudio = [this, client_generation](
        const uint8_t *data, size_t len, int sample_rate, int frame_duration_ms, uint32_t) {
        ScopedLifecycleAdmission user(_lifecycle_refs);
        if (!user) return;
        ScopedOptionalCounter async_user(&_async_post_users);
        if (_chat_generation.load() != client_generation || _shutdown.load() ||
            !_accept_playback.load() || _channel_state.load() != ChannelState::Open) return;
        uint32_t turn = _turn_generation.load();
        if (!_audio.writeOpus(data, len,
                sample_rate > 0 ? sample_rate : CHAT_SAMPLE_RATE,
                frame_duration_ms > 0 ? frame_duration_ms : CHAT_FRAME_DURATION_MS,
                [this, client_generation, turn]() {
                    return _accept_playback.load() && !_shutdown.load() &&
                        _chat_generation.load() == client_generation && _turn_generation.load() == turn;
                })) {
            if (!_accept_playback.load() || _turn_generation.load() != turn) return;
            ControlEvent event{};
            event.type = ControlEventType::AudioError;
            event.error = ESP_FAIL;
            event.chat_generation = client_generation;
            event.turn_generation = turn;
            copyString(event.detail, sizeof(event.detail), "Submit Xiaozhi Opus playback");
            postControlEvent(event);
            return;
        }
        _last_audio_packet_tick.store(xTaskGetTickCount());
    };
    callbacks.onError = [this, client_generation](
        esp_err_t error,
        const std::string &source
    ) {
        ControlEvent event = {};
        event.type = ControlEventType::ChatError;
        event.error = error;
        event.chat_generation = client_generation;
        event.turn_generation = _turn_generation.load();
        copyString(event.detail, sizeof(event.detail), source.c_str());
        (void)postControlEvent(event);
    };

    XiaozhiClient::Config protocol_config(
        use_mqtt,
        use_websocket,
        CHAT_SAMPLE_RATE,
        1,
        CHAT_FRAME_DURATION_MS,
        CHAT_OUTPUT_SAMPLE_RATE
    );
    auto *protocol = new (std::nothrow) XiaozhiClient(
        std::move(protocol_config), std::move(callbacks)
    );
    if (!protocol) {
        setError(ESP_ERR_NO_MEM, "Create Xiaozhi protocol");
        schedulePrepare(RETRY_INTERVAL_MS);
        return false;
    }

    _turn_generation.fetch_add(1);
    _protocol.store(protocol);
    _chat_initialized.store(true);
    _chat_started.store(true);
    _transport_connected = false;
    setState(State::Connecting);

    ret = protocol->start();
    if (ret != ESP_OK || !canPrepare()) {
        destroyChat();
        if (ret != ESP_OK) {
            setError(ret, "Start Xiaozhi protocol");
            schedulePrepare(RETRY_INTERVAL_MS);
        }
        return false;
    }

    schedulePrepare(TRANSPORT_RECONNECT_FALLBACK_MS);
    ESP_UTILS_LOGI("Xiaozhi transport started; waiting for connection");
    return true;
}

void XiaozhiApp::destroyChat()
{
    setAudioModes(false, false);
    _uplink_ready.store(false);
    _chat_started.store(false);
    _chat_initialized.store(false);
    _channel_state.store(ChannelState::Closing);

    XiaozhiClient *protocol = nullptr;
    if (_audio_tx_mutex &&
            xSemaphoreTake(_audio_tx_mutex, portMAX_DELAY) == pdTRUE) {
        protocol = _protocol.exchange(nullptr);
        if (protocol) {
            (void)protocol->stop();
        }
        xSemaphoreGive(_audio_tx_mutex);
    } else {
        protocol = _protocol.exchange(nullptr);
    }
    delete protocol;

    _chat_generation.fetch_add(1);
    _turn_generation.fetch_add(1);
    _transport_connected = false;
    _server_goodbye_generation.store(0);
    resetChannelState();
}

bool XiaozhiApp::ensureAudioReady()
{
    if (_audio.isReady()) return true;
    stopActivationPrompt();
    bool opened = _audio.open(
        [this](const uint8_t *data, size_t size) {
            ScopedLifecycleAdmission user(_lifecycle_refs);
            if (!user || _shutdown.load() || !_voice_enabled.load() || !_uplink_ready.load()) return;
            ScopedSemaphoreLock tx(_audio_tx_mutex, pdMS_TO_TICKS(1000));
            if (!tx) return;
            auto *protocol = _protocol.load();
            if (!_uplink_ready.load() || _channel_state.load() != ChannelState::Open || !protocol) return;
            esp_err_t result = protocol->sendAudio(data, size, 0);
            if (result == ESP_OK) {
                _opus_packets_sent.fetch_add(1);
                _uplink_started_tick.store(0);
            } else {
                _uplink_ready.store(false);
                ControlEvent event{};
                event.type = ControlEventType::ChatError;
                event.error = result;
                event.chat_generation = _chat_generation.load();
                event.turn_generation = _turn_generation.load();
                copyString(event.detail, sizeof(event.detail), "Send Xiaozhi audio");
                postControlEvent(event);
            }
        },
        [this](const std::string &word) {
            ControlEvent event{};
            event.type = ControlEventType::WakeWord;
            copyString(event.text, sizeof(event.text), word.c_str());
            postControlEvent(event);
        });
    if (!opened) setError(ESP_FAIL, "Open Xiaozhi audio services");
    return opened;
}

void XiaozhiApp::shutdownAudio()
{
    _uplink_ready.store(false);
    _voice_enabled.store(false);
    _accept_playback.store(false);
    _audio.close();
}

bool XiaozhiApp::setAudioModes(bool wake_word, bool voice)
{
    _uplink_ready.store(false);
    _uplink_started_tick.store(0);
    _voice_enabled.store(false);
    auto mode = wake_word ? XiaozhiAudio::CaptureMode::WakeWord :
                voice ? XiaozhiAudio::CaptureMode::Voice : XiaozhiAudio::CaptureMode::Off;
    bool success = _audio.setCapture(mode);
    _voice_enabled.store(voice && success);
    return success;
}

bool XiaozhiApp::enterReadyAudioMode()
{
    const bool enable_wake_word =
        _visible.load() && _network_ready.load() && _transport_connected &&
        _chat_started.load();
    if (setAudioModes(enable_wake_word, false)) {
        return true;
    }

    ESP_UTILS_LOGE("Failed to enable Xiaozhi wake-word detection");
    shutdownAudio();
    setError(ESP_FAIL, "Enable Xiaozhi wake-word detection");
    schedulePrepare(RETRY_INTERVAL_MS);
    return false;
}

esp_err_t XiaozhiApp::openAudioChannel()
{
    XiaozhiClient *protocol = _protocol.load();
    if (!protocol) {
        return ESP_ERR_INVALID_STATE;
    }

    ChannelState channel = _channel_state.load();
    if (channel == ChannelState::Open) {
        return ESP_OK;
    }
    if (channel != ChannelState::Closed) {
        return ESP_ERR_INVALID_STATE;
    }

    _channel_state.store(ChannelState::Opening);
    esp_err_t ret = protocol->openAudioChannel();
    if (ret != ESP_OK) {
        resetChannelState();
        return ret;
    }

    _channel_state.store(ChannelState::Open);
    _server_goodbye_generation.store(0);
    return ESP_OK;
}

esp_err_t XiaozhiApp::startAutoStopTurn(const char *wake_word)
{
    esp_err_t ret = openAudioChannel();
    if (ret != ESP_OK) {
        return ret;
    }

    _listen_mode = ListenMode::AutoStop;
    _awaiting_response = false;
    _tts_active = false;
    _tts_stop_pending.store(false);
    _accept_playback.store(false);

    if (!_audio_tx_mutex ||
            xSemaphoreTake(_audio_tx_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_INVALID_STATE;
    }

    XiaozhiClient *protocol = _protocol.load();
    if (!protocol || _channel_state.load() != ChannelState::Open) {
        ret = ESP_ERR_INVALID_STATE;
    } else {
        if (wake_word && wake_word[0]) {
            ret = protocol->sendWakeWordDetected(wake_word);
        }
        if (ret == ESP_OK) {
            ret = protocol->sendStartListening(
                XiaozhiClient::ListeningMode::AutoStop
            );
        }
    }
    xSemaphoreGive(_audio_tx_mutex);
    if (ret == ESP_OK) {
        ESP_UTILS_LOGI("AUTO listening turn started");
    }
    return ret;
}

bool XiaozhiApp::startListening(ListenMode mode, const char *wake_word)
{
    auto restore_audio = [this]() {
        const bool can_capture =
            _visible.load() && _network_ready.load() && _transport_connected &&
            _chat_started.load() && _state.load() == State::Ready &&
            _audio.isReady();
        if (can_capture) {
            enterReadyAudioMode();
        } else {
            setAudioModes(false, false);
        }
    };

    const bool auto_stop_mode = mode == ListenMode::AutoStop;
    _accept_playback.store(false);
    _uplink_ready.store(false);
    if (!_network_ready.load() || !_transport_connected ||
            !_chat_started.load()) {
        restore_audio();
        return false;
    }
    if (!ensureAudioReady()) {
        restore_audio();
        schedulePrepare(RETRY_INTERVAL_MS);
        return false;
    }

    XiaozhiClient *protocol = _protocol.load();
    ChannelState channel = _channel_state.load();
    if (!protocol ||
            (channel != ChannelState::Closed &&
             channel != ChannelState::Open)) {
        restore_audio();
        return false;
    }

    _turn_generation.fetch_add(1);
    _opus_packets_sent.store(0);
    if (!resetPlaybackBuffer()) {
        closeChannel(true);
        shutdownAudio();
        setError(ESP_ERR_TIMEOUT, "Reset Xiaozhi playback");
        schedulePrepare(RETRY_INTERVAL_MS);
        restore_audio();
        return false;
    }

    if (_channel_state.load() == ChannelState::Closed) {
        setState(State::Connecting);
    }
    esp_err_t ret = ESP_OK;
    if (auto_stop_mode) {
        ret = startAutoStopTurn(wake_word);
    } else {
        ret = openAudioChannel();
        if (ret == ESP_OK && _audio_tx_mutex &&
                xSemaphoreTake(_audio_tx_mutex, portMAX_DELAY) == pdTRUE) {
            ret = protocol->sendStartListening(
                XiaozhiClient::ListeningMode::ManualStop
            );
            xSemaphoreGive(_audio_tx_mutex);
        } else if (ret == ESP_OK) {
            ret = ESP_ERR_INVALID_STATE;
        }
    }
    if (ret != ESP_OK) {
        closeChannel(true);
        destroyChat();
        setError(ret, "Start Xiaozhi listening");
        schedulePrepare(RETRY_INTERVAL_MS);
        return false;
    }

    _listen_mode = mode;
    _awaiting_response = false;
    _tts_active = false;
    _tts_stop_pending.store(false);
    setState(State::Listening);

    _opus_packets_sent.store(0);

    if (!setAudioModes(false, true)) {
        closeChannel(true);
        shutdownAudio();
        setError(ESP_FAIL, "Enable Xiaozhi voice processing");
        schedulePrepare(RETRY_INTERVAL_MS);
        return false;
    }
    _uplink_ready.store(true);
    _uplink_started_tick.store(xTaskGetTickCount());
    ESP_UTILS_LOGI(
        "Listening started (mode=%s, source=%s)",
        auto_stop_mode ? "auto" : "manual",
        wake_word && wake_word[0] ? "wake" : "button"
    );
    return true;
}

void XiaozhiApp::finishListening()
{
    if (_listen_mode != ListenMode::Manual ||
            _channel_state.load() != ChannelState::Open) {
        return;
    }
    esp_err_t ret = stopUplink(true);
    if (ret != ESP_OK) {
        closeChannel(true);
        destroyChat();
        if (_network_ready.load()) {
            setError(ret, "Stop Xiaozhi listening");
            schedulePrepare(RETRY_INTERVAL_MS);
        }
        return;
    }
    _awaiting_response = true;
    setState(State::Processing);
}

esp_err_t XiaozhiApp::stopUplink(bool notify_server)
{
    setAudioModes(false, false);
    if (!_audio_tx_mutex ||
            xSemaphoreTake(_audio_tx_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t ret = ESP_OK;
    if (notify_server) {
        ret = ESP_ERR_INVALID_STATE;
        XiaozhiClient *protocol = _protocol.load();
        if (_channel_state.load() == ChannelState::Open && protocol) {
            ret = protocol->sendStopListening();
        }
    }
    xSemaphoreGive(_audio_tx_mutex);
    return ret;
}

void XiaozhiApp::cancelManualConversation()
{
    closeChannel(true);
    clearConversationUi();
    setState(State::Ready);
    enterReadyAudioMode();
}

void XiaozhiApp::closeChannel(bool abort_speaking)
{
    setAudioModes(false, false);
    _accept_playback.store(false);
    _turn_generation.fetch_add(1);
    resetPlaybackBuffer();
    XiaozhiClient *protocol = _protocol.load();
    ChannelState channel = _channel_state.load();
    if (protocol && (channel == ChannelState::Open ||
                   channel == ChannelState::Opening)) {
        _channel_state.store(ChannelState::Closing);
        if (_audio_tx_mutex &&
                xSemaphoreTake(_audio_tx_mutex, portMAX_DELAY) == pdTRUE) {
            if (abort_speaking && (_tts_active ||
                                   _state.load() == State::Speaking)) {
                (void)protocol->sendAbortSpeaking(
                    XiaozhiClient::AbortReason::None
                );
            }
            if (_listen_mode != ListenMode::None) {
                (void)protocol->sendStopListening();
            }
            (void)protocol->closeAudioChannel();
            xSemaphoreGive(_audio_tx_mutex);
        }
    }
    resetChannelState();
}

void XiaozhiApp::resetChannelState()
{
    _uplink_ready.store(false);
    _channel_state.store(ChannelState::Closed);
    _accept_playback.store(false);
    _awaiting_response = false;
    _tts_active = false;
    _tts_stop_pending.store(false);
    _drain_event_posted.store(false);
    _restart_transport_after_playback.store(false);
    _listen_mode = ListenMode::None;
    clearConversationUi();
    resetPlaybackBuffer();
}

void XiaozhiApp::handlePlaybackDrained()
{
    _accept_playback.store(false);
    bool ready = playbackDrainReady();
    if (!ready) {
        _accept_playback.store(true);
        _drain_event_posted.store(false);
        return;
    }

    _tts_stop_pending.store(false);
    _drain_event_posted.store(false);
    if (_restart_transport_after_playback.exchange(false)) {
        ESP_UTILS_LOGI(
            "Final playback drained; restarting Xiaozhi transport now"
        );
        destroyChat();
        if (_network_ready.load()) {
            setState(State::Connecting);
            schedulePrepare(0);
        } else {
            setState(State::NetworkRequired);
        }
        return;
    }

    if (!_visible.load() || !_network_ready.load() ||
            _channel_state.load() != ChannelState::Open) {
        return;
    }
    if (_listen_mode == ListenMode::AutoStop) {
        if (!resetPlaybackBuffer()) {
            closeChannel(false);
            shutdownAudio();
            setError(ESP_FAIL, "Reset Xiaozhi decoder after playback");
            schedulePrepare(RETRY_INTERVAL_MS);
            return;
        }

        _turn_generation.fetch_add(1);
        _opus_packets_sent.store(0);
        esp_err_t ret = startAutoStopTurn();
        if (ret != ESP_OK) {
            closeChannel(false);
            destroyChat();
            setError(ret, "Restart Xiaozhi AUTO listening");
            schedulePrepare(RETRY_INTERVAL_MS);
            return;
        }

        _listen_mode = ListenMode::AutoStop;
        _awaiting_response = false;
        setState(State::Listening);
        if (!setAudioModes(false, true)) {
            closeChannel(false);
            shutdownAudio();
            destroyChat();
            setError(ESP_FAIL, "Resume Xiaozhi voice processing");
            schedulePrepare(RETRY_INTERVAL_MS);
            return;
        }
        _uplink_ready.store(true);
        _uplink_started_tick.store(xTaskGetTickCount());
        ESP_UTILS_LOGI("Listening resumed (mode=auto)");
    } else {
        clearConversationUi();
        _listen_mode = ListenMode::None;
        _awaiting_response = false;
        setState(State::Ready);
        enterReadyAudioMode();
    }
}

void XiaozhiApp::checkPlaybackDrained()
{
    if (_drain_event_posted.load() || !playbackDrainReady()) {
        return;
    }
    if (!_drain_event_posted.exchange(true)) {
        ControlEvent event = {};
        event.type = ControlEventType::PlaybackDrained;
        event.chat_generation = _chat_generation.load();
        event.turn_generation = _turn_generation.load();
        if (!postControlEvent(event)) {
            _drain_event_posted.store(false);
        }
    }
}

bool XiaozhiApp::playbackDrainReady() const
{
    if (!_tts_stop_pending.load() || !_audio.isPlaybackDrained()) return false;
    TickType_t now = xTaskGetTickCount();
    return now - _tts_stop_tick.load() >= pdMS_TO_TICKS(PLAYBACK_DRAIN_MS) &&
        now - _last_audio_packet_tick.load() >= pdMS_TO_TICKS(PLAYBACK_DRAIN_MS);
}

bool XiaozhiApp::resetPlaybackBuffer()
{
    return _audio.resetPlayback();
}

const char *XiaozhiApp::stateText(State state)
{
    switch (state) {
    case State::NetworkRequired:
        return xiaozhiUiText(XiaozhiUiText::NetworkRequired);
    case State::Preparing:
        return xiaozhiUiText(XiaozhiUiText::Preparing);
    case State::ActivationRequired:
        return xiaozhiUiText(XiaozhiUiText::ActivationRequired);
    case State::Connecting:
        return xiaozhiUiText(XiaozhiUiText::Connecting);
    case State::Ready:
        return xiaozhiUiText(XiaozhiUiText::Ready);
    case State::Listening:
        return xiaozhiUiText(XiaozhiUiText::Listening);
    case State::Processing:
        return xiaozhiUiText(XiaozhiUiText::Processing);
    case State::Speaking:
        return xiaozhiUiText(XiaozhiUiText::Speaking);
    case State::Error:
        return xiaozhiUiText(XiaozhiUiText::Error);
    }
    return "";
}

bool XiaozhiApp::tickReached(TickType_t now, TickType_t target)
{
    return static_cast<int32_t>(now - target) >= 0;
}

void XiaozhiApp::controlTaskEntry(void *arg)
{
    static_cast<XiaozhiApp *>(arg)->controlLoop();
}

void XiaozhiApp::ipEventHandler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    (void)event_base;
    (void)event_id;
    (void)event_data;
    auto *instance = static_cast<XiaozhiApp *>(arg);
    if (!instance) {
        return;
    }
    ScopedOptionalCounter async_user(&instance->_async_post_users);
    if (!instance->_shutdown.load()) {
        instance->updateNetworkState();
    }
}

void XiaozhiApp::uiTimerCallback(lv_timer_t *timer)
{
    auto *instance = static_cast<XiaozhiApp *>(lv_timer_get_user_data(timer));
    if (instance) {
        instance->refreshUi();
    }
}

void XiaozhiApp::uiActionCallback(void *context)
{
    auto *instance = static_cast<XiaozhiApp *>(context);
    if (!instance) {
        return;
    }
    ControlEvent event = {};
    event.type = ControlEventType::ActionPressed;
    instance->postControlEvent(event);
}

} // namespace esp_brookesia::apps
