/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <new>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "boost/thread.hpp"
#include "esp_brookesia.hpp"
#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "Main"
#include "esp_lib_utils.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_wifi.h"

#include "esp_lv_adapter.h"
#include "esp_hal_boards/display_device.hpp"

#include "esp_brookesia_app_calculator.hpp"
#include "Drawpanel.hpp"
#include "Camera.hpp"
#include "VideoPlayer.hpp"
#include "MusicPlayer.hpp"
#include "SpecAnalyzer.hpp"
#include "Settings.hpp"
#include "XiaozhiApp.hpp"

using namespace esp_brookesia;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems::phone;

constexpr bool EXAMPLE_SHOW_MEM_INFO = false;

static volatile bool s_wifi_connected = false;

static void wifi_status_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        s_wifi_connected = true;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
        s_wifi_connected = false;
    } else if (event_base == WIFI_EVENT &&
               (event_id == WIFI_EVENT_STA_DISCONNECTED || event_id == WIFI_EVENT_STA_STOP)) {
        s_wifi_connected = false;
    }
}

static void register_wifi_status_events()
{
    static bool registered = false;
    if (registered) {
        return;
    }

    // Nothing else creates the default loop yet, so own it here; a later
    // network app that also calls this just gets ESP_ERR_INVALID_STATE.
    esp_err_t ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(ESP_UTILS_LOG_TAG, "Create default event loop failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_status_event_handler, nullptr);
    if (ret != ESP_OK) {
        ESP_LOGW(ESP_UTILS_LOG_TAG, "Register Wi-Fi status handler failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_status_event_handler, nullptr);
    if (ret != ESP_OK) {
        ESP_LOGW(ESP_UTILS_LOG_TAG, "Register IP status handler failed: %s", esp_err_to_name(ret));
        esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_status_event_handler);
        return;
    }

    registered = true;
}

static bool is_wifi_connected()
{
    return s_wifi_connected;
}

extern "C" void app_main(void)
{
    esp_log_level_set("rpc_rsp", ESP_LOG_ERROR);

    ESP_UTILS_LOGI("Display ESP-Brookesia phone demo");

    /* Configure display */
    lv_display_t *disp = esp_hal_boards::DisplayDevice::get_instance().init();
    ESP_UTILS_CHECK_NULL_EXIT(disp, "Start display failed");

    /* Route the Brookesia GUI lock to the esp_lvgl_adapter mutex */
    LvLock::registerCallbacks([](int timeout_ms) {
        ESP_UTILS_CHECK_FALSE_RETURN(esp_lv_adapter_lock(timeout_ms) == ESP_OK, false,
                                     "Lock failed (timeout_ms: %d)", timeout_ms);
        return true;
    }, []() {
        esp_lv_adapter_unlock();
        return true;
    });

    /* Init NVS */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(ESP_UTILS_LOG_TAG, "NVS requires erase before initialization: %s", esp_err_to_name(err));
        err = nvs_flash_erase();
        if (err != ESP_OK) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "NVS erase failed: %s", esp_err_to_name(err));
            return;
        }
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(ESP_UTILS_LOG_TAG, "NVS initialization failed: %s", esp_err_to_name(err));
        return;
    }

    /* Create a phone object */
    Phone *phone = new (std::nothrow) Phone(disp);
    ESP_UTILS_CHECK_NULL_EXIT(phone, "Create phone failed");

    {
        // When operating on non-GUI tasks, should acquire a lock before operating on LVGL
        LvLockGuard gui_guard;

        const auto &stylesheet = ESP_BROOKESIA_PHONE_800_1280_DARK_STYLESHEET();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->addStylesheet(stylesheet), "Add stylesheet failed");
        ESP_UTILS_CHECK_FALSE_EXIT(phone->activateStylesheet(stylesheet), "Activate stylesheet failed");

        /* Begin the phone */
        ESP_UTILS_CHECK_FALSE_EXIT(phone->begin(), "Begin failed");
        // assert(phone->getDisplay().showContainerBorder() && "Show container border failed");
    }

    // Light the panel only after the home screen has had a frame to render,
    // instead of flashing an empty LVGL screen.
    vTaskDelay(pdMS_TO_TICKS(50));
    err = esp_hal_boards::display_backlight_set(100);
    if (err != ESP_OK) {
        ESP_LOGW(ESP_UTILS_LOG_TAG, "Backlight on failed: %s", esp_err_to_name(err));
    }

    /* Init and install apps from registry */
    {
        std::vector<systems::base::Manager::RegistryAppInfo> inited_apps;
        ESP_UTILS_CHECK_FALSE_EXIT(phone->initAppFromRegistry(inited_apps), "Init app registry failed");
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installAppFromRegistry(inited_apps), "Install app registry failed");
    }

    /* Install local apps */
    {
        auto calculator_app = esp_brookesia::apps::Calculator::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(calculator_app), "Start Calculator failed");

        auto drawpanel_app = esp_brookesia::apps::Drawpanel::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(drawpanel_app), "Start DrawPanel failed");

        auto camera_app = esp_brookesia::apps::Camera::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(camera_app), "Start Camera failed");

        auto setting_app = esp_brookesia::apps::Settings::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(setting_app), "Start Camera failed");

        auto video_app = esp_brookesia::apps::VideoPlayer::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(video_app), "Start Camera failed");

        auto music_app = esp_brookesia::apps::MusicPlayer::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(music_app), "Install MusicPlayer failed");

        auto specanalyzer_app = esp_brookesia::apps::SpecAnalyzer::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(specanalyzer_app), "Start Camera failed");

        auto xiaozhi_app = esp_brookesia::apps::XiaozhiApp::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(xiaozhi_app), "Start Camera failed");

    }

    register_wifi_status_events();

    {
        LvLockGuard gui_guard;

        /* Create a timer to update the clock */
        lv_timer_t *status_timer = lv_timer_create([](lv_timer_t *t) {
            time_t now;
            struct tm timeinfo;
            Phone *phone = (Phone *)t->user_data;

            ESP_UTILS_CHECK_NULL_EXIT(phone, "Invalid phone");

            time(&now);
            localtime_r(&now, &timeinfo);

            ESP_UTILS_CHECK_FALSE_EXIT(
                phone->getDisplay().getStatusBar()->setClock(timeinfo.tm_hour, timeinfo.tm_min),
                "Refresh status bar failed"
            );

            bool connected = is_wifi_connected();
            StatusBar::WifiState wifi_state = connected ? StatusBar::WifiState::SIGNAL_3 : StatusBar::WifiState::DISCONNECTED;

            ESP_UTILS_CHECK_FALSE_EXIT(
                phone->getDisplay().getStatusBar()->setWifiIconState(wifi_state),
                "Refresh status bar failed"
            );
        }, 1000, phone);
        ESP_UTILS_CHECK_NULL_EXIT(status_timer, "Create status timer failed");
    }

    if constexpr(EXAMPLE_SHOW_MEM_INFO) {
        esp_utils::thread_config_guard thread_config({
            .name = "mem_info",
            .stack_size = 4096,
        });
        boost::thread([ = ]() {
            char buffer[128];    /* Make sure buffer is enough for `sprintf` */
            size_t internal_free = 0;
            size_t internal_total = 0;
            size_t external_free = 0;
            size_t external_total = 0;

            while (1) {
                internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
                internal_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
                external_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                external_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
                snprintf(
                    buffer,
                    sizeof(buffer),
                    "\t           Biggest /     Free /    Total\n"
                    "\t  SRAM : [%8zu / %8zu / %8zu]\n"
                    "\t PSRAM : [%8zu / %8zu / %8zu]",
                    heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), internal_free, internal_total,
                    heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM), external_free, external_total
                );
                ESP_UTILS_LOGI("\n%s", buffer);

                {
                    LvLockGuard gui_guard;
                    ESP_UTILS_CHECK_FALSE_EXIT(
                        phone->getDisplay().getRecentsScreen()->setMemoryLabel(
                            internal_free / 1024, internal_total / 1024, external_free / 1024, external_total / 1024
                        ), "Set memory label failed"
                    );
                }

                boost::this_thread::sleep_for(boost::chrono::seconds(5));
            }
        }).detach();
    }
}
