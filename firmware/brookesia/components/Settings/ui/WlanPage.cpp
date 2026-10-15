#include "WlanPage.hpp"
#include "../Settings.hpp"
#include "SettingsUI.hpp"

#define ESP_UTILS_LOG_TAG "BS:WlanPage"
#include "esp_lib_utils.h"
#include "nvs.h"
#include "brookesia/lib_utils/describe_helpers.hpp"

#include <stdlib.h>
#include <algorithm>
#include <string>
#include <new>

LV_IMG_DECLARE(wifi_1);
LV_IMG_DECLARE(wifi_2);
LV_IMG_DECLARE(wifi_3);
LV_IMG_DECLARE(wifi_4);

namespace esp_brookesia::apps
{
    WlanPage *WlanPage::_instance = nullptr;

    lv_obj_t *WlanPage::ta = nullptr;
    lv_obj_t *WlanPage::kb = nullptr;

    WlanPage *WlanPage::requestInstance(bool use_status_bar, bool use_navigation_bar)
    {
        if (_instance == nullptr)
        {
            _instance = new WlanPage(use_status_bar, use_navigation_bar);
        }
        return _instance;
    }

    WlanPage::WlanPage(bool use_status_bar, bool use_navigation_bar)
        : App("WLAN", nullptr, true, use_status_bar, use_navigation_bar),
          page_root(nullptr), label(nullptr), list1(nullptr),
          connected_text(nullptr), conn_btn(nullptr), available_text(nullptr)
    {
        status_btn = nullptr;
        wlan_switch = nullptr;
        wifi_icon = nullptr;
        spinner = nullptr;
        password_title = nullptr;
    }

    WlanPage::~WlanPage() {}

    bool WlanPage::run()
    {
        ESP_UTILS_LOGI("WlanPage Run");
        page_active = true;
        scan_running = false;
        scan_failed = false;
        auto_connect_waiting = false;
        auto_connect_check_pending = false;
        password_editing = false;

        _nvs_param_map[NVS_KEY_WIFI_ENABLE] = false;
        loadNvsParam();

        subscribe_wifi_events();
        CreateWifiUI();

        // Restore the persisted Wi-Fi switch state after the UI exists.
        if (_nvs_param_map[NVS_KEY_WIFI_ENABLE]) {
            OpenWifi();
        }

        return true;
    }

    bool WlanPage::back()
    {
        ESP_UTILS_LOGI("WlanPage Back");
        
        Settings::requestInstance()->showRootPage();
        return true;
    }

    bool WlanPage::close()
    {
        ESP_UTILS_LOGI("WlanPage Close");
        page_active = false;
        scan_start_pending = false;
        scan_running = false;
        auto_connect_waiting = false;
        auto_connect_check_pending = false;
        WifiHelper::call_function_async(WifiHelper::FunctionId::TriggerScanStop);
        unsubscribe_wifi_events();
        if (page_root != nullptr)
        {
            lv_obj_del(page_root);
            page_root = nullptr;
            label = nullptr;
            list1 = nullptr;
            connected_text = nullptr;
            conn_btn = nullptr;
            available_text = nullptr;
            status_btn = nullptr;
            wlan_switch = nullptr;
            wifi_icon = nullptr;
            spinner = nullptr;
            password_title = nullptr;
            ta = nullptr;
            kb = nullptr;
            wifi_btns.clear();
            ap_rows.clear();
            scan_status = nullptr;
            refresh_btn = nullptr;
            empty_text = nullptr;
            connection_spinner = nullptr;
            settings_ui::reset_list_styles(style_list, style_list_btn, style_list_text, style_list_btn_pressed);
        }

        return true;
    }


    void WlanPage::CreateWifiUI()
    {
        lv_obj_clean(lv_scr_act());

        page_root = settings_ui::create_page(lv_scr_act(), *Settings::requestInstance());
        status_btn = settings_ui::create_header(page_root, "WLAN", [](lv_event_t *e) {
            (void)e;
            lv_async_call([](void *param) {
                (void)param;
                Settings::requestInstance()->showRootPage();
            }, nullptr);
        });

        settings_ui::init_list_styles(style_list, style_list_btn, style_list_text, style_list_btn_pressed);

        list1 = settings_ui::create_content_list(page_root);
        lv_obj_add_style(list1, &style_list, LV_PART_MAIN);

        settings_ui::add_section(list1, "Network", style_list_text);

        lv_obj_t *wlan_btn = lv_list_add_button(list1, nullptr, "Wi-Fi");
        lv_obj_add_style(wlan_btn, &style_list_btn, LV_PART_MAIN);
        settings_ui::use_ellipsis_for_button_label(wlan_btn);

        wlan_switch = lv_switch_create(wlan_btn);
        lv_obj_set_size(wlan_switch, 64, 36);
        lv_obj_remove_state(wlan_switch, LV_STATE_CHECKED);
        
        lv_obj_add_event_cb(wlan_switch, [](lv_event_t *e)
        {
            lv_obj_t *sw = static_cast<lv_obj_t *>(lv_event_get_target(e));
            bool checked = lv_obj_has_state(sw, LV_STATE_CHECKED);
            auto page = WlanPage::requestInstance();

            if (checked) page->toggleWifiUI(WIFIOPEN);
            else page->toggleWifiUI(WIFICLOSE);
        }, 
        LV_EVENT_VALUE_CHANGED, nullptr);

        lv_obj_remove_flag(wlan_btn, LV_OBJ_FLAG_CLICKABLE);
        // lv_obj_add_state(conn_btn, LV_STATE_DISABLED);

        lv_obj_update_layout(page_root);
        lv_coord_t content_width = lv_obj_get_width(page_root) - settings_ui::PAGE_HORIZONTAL_MARGIN * 2;
        lv_coord_t keyboard_height = lv_obj_get_height(page_root) * 48 / 100;
        if (content_width < 1) {
            content_width = 1;
        }
        if (keyboard_height < 1) {
            keyboard_height = 1;
        }

        connected_text = settings_ui::add_section(list1, "Connected WLAN", style_list_text);
        conn_btn = lv_list_add_button(list1, nullptr, "SSID");
        lv_obj_add_style(conn_btn, &style_list_btn, LV_PART_MAIN);
        settings_ui::use_ellipsis_for_button_label(conn_btn);
        wifi_icon = lv_label_create(conn_btn);
        lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_20, LV_PART_MAIN);
        connection_spinner = lv_spinner_create(conn_btn);
        lv_obj_set_size(connection_spinner, 24, 24);
        lv_obj_set_style_arc_width(connection_spinner, 3, LV_PART_MAIN);
        lv_obj_set_style_arc_width(connection_spinner, 3, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(connection_spinner, lv_color_hex(settings_ui::COLOR_ACCENT), LV_PART_INDICATOR);
        lv_obj_add_event_cb(conn_btn, [](lv_event_t *e) {
            auto page = static_cast<WlanPage *>(lv_event_get_user_data(e));
            if (page->Wifi_state != DISCONNECT) return;
            page->Wifi_state = CONNECTING;
            page->scan_running = false;
            WifiHelper::call_function_async(WifiHelper::FunctionId::TriggerScanStop);
            page->update_network_ui();
            const bool submitted = WifiHelper::call_function_async(
                WifiHelper::FunctionId::TriggerGeneralAction,
                BROOKESIA_DESCRIBE_TO_STR(WifiHelper::GeneralAction::Connect),
                [page](service::FunctionResult &&result) {
                    if (!result.success && page->Wifi_state != WIFICLOSE) {
                        page->Wifi_state = DISCONNECT;
                        if (page->page_active) lv_async_call(wifi_state_cb, page);
                    }
                });
            if (!submitted) {
                page->Wifi_state = DISCONNECT;
                page->update_network_ui();
            }
        }, LV_EVENT_SHORT_CLICKED, this);

        available_text = lv_obj_create(list1);
        lv_obj_remove_style_all(available_text);
        lv_obj_set_width(available_text, lv_pct(100));
        lv_obj_set_height(available_text, 60);
        lv_obj_set_flex_flow(available_text, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(available_text, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(available_text, 12, LV_PART_MAIN);
        lv_obj_clear_flag(available_text, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t *available_title = lv_label_create(available_text);
        lv_label_set_text(available_title, "Available WLAN");
        lv_obj_set_style_text_font(available_title, &lv_font_montserrat_20, LV_PART_MAIN);
        lv_obj_set_style_text_color(available_title, lv_color_hex(settings_ui::COLOR_SECONDARY_TEXT), LV_PART_MAIN);
        lv_obj_set_flex_grow(available_title, 1);
        spinner = lv_spinner_create(available_text);
        lv_obj_set_size(spinner, 24, 24);
        lv_obj_set_style_arc_width(spinner, 3, LV_PART_MAIN);
        lv_obj_set_style_arc_width(spinner, 3, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(spinner, lv_color_hex(settings_ui::COLOR_ACCENT), LV_PART_INDICATOR);
        scan_status = lv_label_create(available_text);
        lv_obj_set_style_text_font(scan_status, &lv_font_montserrat_20, LV_PART_MAIN);
        lv_obj_set_style_text_color(scan_status, lv_color_hex(settings_ui::COLOR_SECONDARY_TEXT), LV_PART_MAIN);
        refresh_btn = lv_button_create(available_text);
        lv_obj_set_size(refresh_btn, 108, 44);
        lv_obj_set_style_bg_color(refresh_btn, lv_color_hex(settings_ui::COLOR_SURFACE), LV_PART_MAIN);
        lv_obj_t *refresh_label = lv_label_create(refresh_btn);
        lv_label_set_text(refresh_label, "Refresh");
        lv_obj_set_style_text_font(refresh_label, &lv_font_montserrat_20, LV_PART_MAIN);
        lv_obj_center(refresh_label);
        lv_obj_add_event_cb(refresh_btn, [](lv_event_t *e) {
            static_cast<WlanPage *>(lv_event_get_user_data(e))->start_wifi_scan(true);
        }, LV_EVENT_CLICKED, this);
        empty_text = settings_ui::add_section(list1, "", style_list_text);
        rebuild_network_list();

        password_title = lv_label_create(page_root);
        lv_label_set_text(password_title, "Connect to network");
        lv_label_set_long_mode(password_title, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_width(password_title, content_width);
        lv_obj_set_style_text_font(password_title, &lv_font_montserrat_24, LV_PART_MAIN);
        lv_obj_set_style_text_color(password_title, lv_color_hex(settings_ui::COLOR_PRIMARY_TEXT), LV_PART_MAIN);
        lv_obj_align(password_title, LV_ALIGN_TOP_MID, 0, settings_ui::PAGE_CONTENT_TOP + 20);
        lv_obj_add_flag(password_title, LV_OBJ_FLAG_HIDDEN);

        ta = lv_textarea_create(page_root);
        lv_obj_add_flag(ta, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(ta, content_width, 64);
        lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, settings_ui::PAGE_CONTENT_TOP + 64);

        lv_textarea_set_password_mode(ta, true);
        lv_textarea_set_max_length(ta, sizeof(wifi_pwd) - 1);
        lv_textarea_set_password_show_time(ta, 1500);
        lv_textarea_set_placeholder_text(ta, "Enter password...");
        lv_obj_set_style_bg_color(ta, lv_color_hex(settings_ui::COLOR_SURFACE), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(ta, lv_color_hex(settings_ui::COLOR_BORDER), LV_PART_MAIN);
        lv_obj_set_style_border_width(ta, 2, LV_PART_MAIN);
        lv_obj_set_style_radius(ta, 6, LV_PART_MAIN);
        lv_obj_set_style_text_color(ta, lv_color_hex(settings_ui::COLOR_PRIMARY_TEXT), LV_PART_MAIN);
        lv_obj_set_style_text_color(ta, lv_color_hex(settings_ui::COLOR_SECONDARY_TEXT), LV_PART_TEXTAREA_PLACEHOLDER);
        lv_obj_set_style_text_font(ta, &lv_font_montserrat_20, LV_PART_MAIN);
        lv_obj_set_style_text_font(ta, &lv_font_montserrat_20, LV_PART_TEXTAREA_PLACEHOLDER);
        lv_obj_set_style_bg_color(ta, lv_color_white(), LV_PART_CURSOR);
        lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, LV_PART_CURSOR);
        lv_obj_add_event_cb(ta, ta_event_cb, LV_EVENT_ALL, this);

        kb = lv_keyboard_create(page_root);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(kb, content_width, keyboard_height);
        lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, -16);
        lv_obj_set_style_bg_color(kb, lv_color_hex(0x181818), LV_PART_MAIN);
        lv_obj_set_style_border_color(kb, lv_color_hex(settings_ui::COLOR_BORDER), LV_PART_MAIN);
        lv_obj_set_style_border_width(kb, 2, LV_PART_MAIN);
        lv_obj_set_style_radius(kb, 6, LV_PART_MAIN);
        lv_obj_set_style_bg_color(kb, lv_color_hex(0x333333), LV_PART_ITEMS);
        lv_obj_set_style_text_color(kb, lv_color_hex(settings_ui::COLOR_PRIMARY_TEXT), LV_PART_ITEMS);
        lv_obj_set_style_text_font(kb, &lv_font_montserrat_20, LV_PART_ITEMS);
        lv_obj_set_style_border_color(kb, lv_color_hex(0x555555), LV_PART_ITEMS);
        lv_obj_set_style_border_width(kb, 1, LV_PART_ITEMS);
        lv_obj_set_style_radius(kb, 4, LV_PART_ITEMS);
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_ALL, this);
    }

    bool WlanPage::OpenWifi()
    {
        ESP_UTILS_LOGI("WlanPage OpenWifi");

        if (wlan_switch) {
            lv_obj_add_state(wlan_switch, LV_STATE_CHECKED);
            lv_obj_add_state(wlan_switch, LV_STATE_DISABLED);
        }
        Wifi_state = WIFIOPEN;
        auto_connect_waiting = false;
        scan_start_pending = false;
        update_network_ui();

        const bool submitted = WifiHelper::call_function_async(
            WifiHelper::FunctionId::TriggerGeneralAction,
            BROOKESIA_DESCRIBE_TO_STR(WifiHelper::GeneralAction::Start),
            [this](service::FunctionResult &&result) {
                if (!result.success) {
                    ESP_UTILS_LOGW("Start Wi-Fi service failed: %s", result.error_message.c_str());
                    Wifi_state = DISCONNECT;
                    if (page_active) {
                        lv_async_call(wifi_state_cb, this);
                    }
                    return;
                }

                // If Wi-Fi was already started before this page opened, no new
                // Started event is guaranteed. Querying the state covers that case.
                refresh_wifi_state();
            });
        if (!submitted) {
            ESP_UTILS_LOGW("Submit Wi-Fi start request failed");
            Wifi_state = DISCONNECT;
            lv_async_call(wifi_state_cb, this);
            return false;
        }

        return true;
    }

    bool WlanPage::CloseWifi()
    {
        ESP_UTILS_LOGI("WlanPage CloseWifi");
        Wifi_state = WIFICLOSE;
        auto_connect_waiting = false;
        scan_start_pending = false;

        WifiHelper::call_function_async(WifiHelper::FunctionId::TriggerScanStop);
        const bool submitted = WifiHelper::call_function_async(
            WifiHelper::FunctionId::TriggerGeneralAction,
            BROOKESIA_DESCRIBE_TO_STR(WifiHelper::GeneralAction::Stop),
            [this](service::FunctionResult &&result) {
                if (!result.success) {
                    ESP_UTILS_LOGW("Stop Wi-Fi service failed: %s", result.error_message.c_str());
                }
            });
        if (!submitted) {
            ESP_UTILS_LOGW("Submit Wi-Fi stop request failed");
        }
        
        scan_running = false;
        password_editing = false;
        if (kb) lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        if (ta) lv_obj_add_flag(ta, LV_OBJ_FLAG_HIDDEN);
        if (password_title) lv_obj_add_flag(password_title, LV_OBJ_FLAG_HIDDEN);
        update_network_ui();

        return true;
    }

    void WlanPage::toggleWifiUI(WifiState visible)
    {
        if (visible == WIFIOPEN) 
        {
            _nvs_param_map[NVS_KEY_WIFI_ENABLE] = true;
            setNvsParam(NVS_KEY_WIFI_ENABLE, true);
            printf("WIFIOPEN \r\n");
            OpenWifi();
        }
        else if(visible == WIFICLOSE) {
            printf("WIFICLOSE \r\n");
            _nvs_param_map[NVS_KEY_WIFI_ENABLE] = false;
            setNvsParam(NVS_KEY_WIFI_ENABLE, false);
            CloseWifi();
        }
    }
    // Runs on the LVGL thread after Wi-Fi state changes.
    void WlanPage::wifi_state_cb(void *param)
    {
        WlanPage *self = static_cast<WlanPage *>(param);
        if (!self || !self->page_active || !self->list1) {
            return;
        }
        if (!self->password_editing || self->Wifi_state == WIFICLOSE) {
            self->password_editing = false;
            if (self->kb) lv_obj_add_flag(self->kb, LV_OBJ_FLAG_HIDDEN);
            if (self->ta) lv_obj_add_flag(self->ta, LV_OBJ_FLAG_HIDDEN);
            if (self->password_title) lv_obj_add_flag(self->password_title, LV_OBJ_FLAG_HIDDEN);
        }
        self->update_network_ui();
    }

    void WlanPage::scan_ui_cb(void *param)
    {
        auto page = static_cast<WlanPage *>(param);
        if (page && page->page_active && page->list1) page->update_network_ui();
    }

    // Handles AP list button events and owns the per-button user_data lifetime.
    void WlanPage::wifi_btn_cb(lv_event_t * e)
    {
        lv_event_code_t event_code = lv_event_get_code(e);
        btn_user_data_t *ud = (btn_user_data_t *)lv_event_get_user_data(e);
        if (!ud) {
            return;
        }
        if (event_code == LV_EVENT_DELETE) {
            delete ud;
            return;
        }
        WlanPage *page = static_cast<WlanPage *>(ud->self);
        if (!page || !page->page_active) {
            return;
        }
        // When a network in the list is pressed
        if(event_code == LV_EVENT_SHORT_CLICKED) {
            if (page->Wifi_state == CONNECTING || page->Wifi_state == WIFICLOSE) return;
            const WifiScanApInfo selected_ap = ud->ap;
            page->selected_ssid = selected_ap.ssid;
            page->selected_ap_is_locked = selected_ap.is_locked;
            ESP_UTILS_LOGI("Selected SSID: %s", page->selected_ssid.c_str());

            if (!selected_ap.is_locked) {
                page->wifi_pwd[0] = '\0';
                page->connect_selected_ap();
                return;
            }

            if (page->password_title) {
                lv_label_set_text_fmt(page->password_title, "Connect to %s", page->selected_ssid.c_str());
                lv_obj_remove_flag(page->password_title, LV_OBJ_FLAG_HIDDEN);
            }
            page->password_editing = true;
            lv_textarea_set_text(ta, "");
            lv_obj_add_flag(page->list1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ta, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ta, LV_STATE_FOCUSED);
            lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
            lv_keyboard_set_textarea(kb, ta);

        }
    }

    // Preserve the fixed controls and scroll position while replacing AP rows.
    void WlanPage::rebuild_network_list()
    {
        if (!page_active || !list1) return;
        const lv_coord_t scroll_y = lv_obj_get_scroll_y(list1);
        for (auto button : wifi_btns) lv_obj_del(button);
        wifi_btns.clear();
        ap_rows.clear();
        std::vector<WifiScanApInfo> scanned_aps;
        {
            std::lock_guard<std::mutex> lock(ap_info_mutex);
            scanned_aps = ap_info;
        }
        for (const auto &ap : scanned_aps) {
            if (wifi_btns.size() >= DEFAULT_SCAN_LIST_SIZE) break;
            if (ap.ssid.empty()) continue;
            auto ud = new (std::nothrow) btn_user_data_t{ap, this};
            if (!ud) continue;
            lv_obj_t *button = lv_list_add_button(list1, nullptr, ap.ssid.c_str());
            lv_obj_add_style(button, &style_list_btn, LV_PART_MAIN);
            lv_obj_add_style(button, &style_list_btn_pressed, LV_STATE_PRESSED);
            settings_ui::use_ellipsis_for_button_label(button);
            lv_obj_t *icon = lv_image_create(button);
            lv_img_set_src(icon, ap.rssi >= -25 ? &wifi_4 : ap.rssi >= -50 ? &wifi_3 : ap.rssi >= -75 ? &wifi_2 : &wifi_1);
            lv_obj_t *status = lv_label_create(button);
            lv_label_set_text(status, "Connecting...");
            lv_obj_set_style_text_font(status, &lv_font_montserrat_20, LV_PART_MAIN);
            lv_obj_t *progress = lv_spinner_create(button);
            lv_obj_set_size(progress, 24, 24);
            lv_obj_set_style_arc_width(progress, 3, LV_PART_MAIN);
            lv_obj_set_style_arc_width(progress, 3, LV_PART_INDICATOR);
            lv_obj_set_style_arc_color(progress, lv_color_hex(settings_ui::COLOR_ACCENT), LV_PART_INDICATOR);
            lv_obj_add_event_cb(button, wifi_btn_cb, LV_EVENT_ALL, ud);
            wifi_btns.push_back(button);
            ap_rows.push_back({button, progress, status, ap.ssid});
        }
        update_network_ui();
        lv_obj_update_layout(list1);
        lv_obj_scroll_to_y(list1, scroll_y, LV_ANIM_OFF);
    }

    void WlanPage::update_network_ui()
    {
        if (!page_active || !list1) return;
        const WifiState state = Wifi_state.load();
        const bool enabled = state != WIFICLOSE;
        const bool connecting = state == CONNECTING;
        const bool waiting = enabled && auto_connect_waiting;
        const bool scanning = enabled && !connecting && !waiting && (scan_running || scan_start_pending);
        auto show = [](lv_obj_t *obj, bool visible) {
            if (!obj) return;
            if (visible) lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        };
        show(list1, !password_editing);
        lv_obj_remove_state(wlan_switch, LV_STATE_DISABLED);
        if (enabled) lv_obj_add_state(wlan_switch, LV_STATE_CHECKED);
        else lv_obj_remove_state(wlan_switch, LV_STATE_CHECKED);
        std::string ssid;
        {
            std::lock_guard<std::mutex> lock(wifi_ssid_mutex);
            ssid = wifi_ssid;
        }
        size_t available_count = 0;
        for (const auto &row : ap_rows) {
            const bool selected = connecting && row.ssid == ssid;
            // Keep the cached row so it reappears after switching networks.
            const bool available = enabled && !(state == CONNECTED && row.ssid == ssid);
            show(row.button, available);
            if (available) ++available_count;
            show(row.progress, selected);
            show(row.status, selected);
            if (connecting) lv_obj_add_state(row.button, LV_STATE_DISABLED);
            else lv_obj_remove_state(row.button, LV_STATE_DISABLED);
        }
        const bool show_connection = enabled && !ssid.empty() &&
            (state == CONNECTED || state == DISCONNECT || waiting || connecting);
        show(connected_text, show_connection);
        show(conn_btn, show_connection);
        lv_label_set_text(connected_text, state == CONNECTED ? "Connected WLAN" : "Connection status");
        lv_list_set_button_text(list1, conn_btn, ssid.c_str());
        lv_label_set_text(wifi_icon, state == CONNECTED ? LV_SYMBOL_OK : connecting ? "Connecting..." :
            waiting ? "Auto-connect pending" : "Disconnected");
        show(connection_spinner, show_connection && connecting);
        if (state == DISCONNECT) lv_obj_add_flag(conn_btn, LV_OBJ_FLAG_CLICKABLE);
        else lv_obj_remove_flag(conn_btn, LV_OBJ_FLAG_CLICKABLE);
        show(available_text, enabled);
        show(spinner, scanning);
        show(scan_status, scanning || scan_failed);
        lv_label_set_text(scan_status, scanning ? "Scanning..." : "Scan failed");
        if (scanning || connecting || state == WIFIOPEN) lv_obj_add_state(refresh_btn, LV_STATE_DISABLED);
        else lv_obj_remove_state(refresh_btn, LV_STATE_DISABLED);
        show(empty_text, enabled && available_count == 0);
        lv_label_set_text(empty_text, scanning ? "Searching for networks..." :
            scan_failed ? "Scan failed. Tap Refresh to retry." :
            waiting ? "Waiting for automatic connection..." :
            state == CONNECTED ? "Tap Refresh to find other networks." :
            state == WIFIOPEN ? "Starting Wi-Fi..." : connecting ? "Connecting to network..." :
            "No networks found. Tap Refresh to retry.");
    }

    // Keyboard READY/CANCEL hides the input and submits the connection to the service.
    void WlanPage::kb_event_cb(lv_event_t *e)
    {
        WlanPage *self = (WlanPage *)lv_event_get_user_data(e);
        lv_event_code_t code = lv_event_get_code(e);

        if (code == LV_EVENT_CANCEL) {
            self->password_editing = false;
            lv_obj_add_flag(self->kb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(self->ta, LV_OBJ_FLAG_HIDDEN);
            if (self->password_title) {
                lv_obj_add_flag(self->password_title, LV_OBJ_FLAG_HIDDEN);
            }
            lv_obj_remove_flag(self->list1, LV_OBJ_FLAG_HIDDEN);
            lv_textarea_set_text(self->ta, "");
            self->update_network_ui();
            return;
        }

        if (code == LV_EVENT_READY) {
            strlcpy(self->wifi_pwd, lv_textarea_get_text(ta), sizeof(self->wifi_pwd));
            ESP_UTILS_LOGI("Wi-Fi password length: %u", static_cast<unsigned>(strlen(self->wifi_pwd)));
            self->connect_selected_ap();
        }
    }

    // Textarea focus controls keyboard visibility.
    void WlanPage::ta_event_cb(lv_event_t * e) 
    {
        WlanPage *self = (WlanPage *)lv_event_get_user_data(e);
        lv_obj_t * ta = (lv_obj_t*)lv_event_get_target(e);

        if (lv_event_get_code(e) == LV_EVENT_FOCUSED) {
            lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
            lv_keyboard_set_textarea(kb, ta);
        }
        else if (lv_event_get_code(e) == LV_EVENT_DEFOCUSED) {
            self->password_editing = false;
            lv_obj_remove_flag(self->list1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ta, LV_OBJ_FLAG_HIDDEN);
            if (self->password_title) {
                lv_obj_add_flag(self->password_title, LV_OBJ_FLAG_HIDDEN);
            }
            lv_keyboard_set_textarea(kb, NULL);
        }
    }

    void WlanPage::subscribe_wifi_events()
    {
        if (!action_event_connection.connected()) {
            action_event_connection = WifiHelper::subscribe_event(
                WifiHelper::EventId::GeneralActionTriggered,
                [this](const std::string &event_name, const std::string &action) {
                    (void)event_name;
                    if (!page_active || Wifi_state == WIFICLOSE || action != "Connect") return;
                    auto_connect_waiting = false;
                    Wifi_state = CONNECTING;
                    scan_running = false;
                    WifiHelper::call_function_async(WifiHelper::FunctionId::TriggerScanStop);
                    refresh_connected_ap();
                    lv_async_call(scan_ui_cb, this);
                });
        }
        if (!general_event_connection.connected()) {
            general_event_connection = WifiHelper::subscribe_event(
                WifiHelper::EventId::GeneralEventHappened,
                [this](const std::string &event_name, const std::string &event, bool is_unexpected) {
                    (void)event_name;
                    on_general_event(event, is_unexpected);
                });
        }
        if (!scan_event_connection.connected()) {
            scan_event_connection = WifiHelper::subscribe_event(
                WifiHelper::EventId::ScanApInfosUpdated,
                [this](const std::string &event_name, const boost::json::array &ap_infos) {
                    (void)event_name;
                    on_scan_ap_infos(ap_infos);
                });
        }
        if (!scan_state_connection.connected()) {
            scan_state_connection = WifiHelper::subscribe_event(
                WifiHelper::EventId::ScanStateChanged,
                [this](const std::string &event_name, bool is_running) {
                    (void)event_name;
                    scan_running = is_running;
                    if (page_active) lv_async_call(scan_ui_cb, this);
                });
        }
        if (!action_event_connection.connected() || !general_event_connection.connected() || !scan_event_connection.connected() ||
            !scan_state_connection.connected()) {
            ESP_UTILS_LOGW("Subscribe to Wi-Fi service events failed");
        }
    }

    void WlanPage::unsubscribe_wifi_events()
    {
        general_event_connection.disconnect();
        action_event_connection.disconnect();
        scan_event_connection.disconnect();
        scan_state_connection.disconnect();
    }

    void WlanPage::on_general_event(const std::string &event, bool is_unexpected)
    {
        ESP_UTILS_LOGI("Wi-Fi service event: %s", event.c_str());
        if (event == BROOKESIA_DESCRIBE_TO_STR(WifiHelper::GeneralEvent::Started)) {
            if (Wifi_state != WIFICLOSE) {
                refresh_wifi_state();
            }
            return;
        }

        if (event == BROOKESIA_DESCRIBE_TO_STR(WifiHelper::GeneralEvent::Connected)) {
            auto_connect_waiting = false;
            Wifi_state = CONNECTED;
            scan_running = false;
            WifiHelper::call_function_async(WifiHelper::FunctionId::TriggerScanStop);
            refresh_connected_ap();
        } else if (event == BROOKESIA_DESCRIBE_TO_STR(WifiHelper::GeneralEvent::Disconnected)) {
            // A non-unexpected disconnect is part of an intentional AP switch.
            // Keep the connecting indicator until the service reports the result.
            if (Wifi_state != WIFICLOSE && (Wifi_state != CONNECTING || is_unexpected)) {
                auto_connect_waiting = false;
                Wifi_state = DISCONNECT;
            } else if (Wifi_state == CONNECTING) {
                return;
            }
        } else if (event == BROOKESIA_DESCRIBE_TO_STR(WifiHelper::GeneralEvent::Stopped)) {
            auto_connect_waiting = false;
            Wifi_state = WIFICLOSE;
        } else {
            return;
        }

        if (page_active) {
            lv_async_call(wifi_state_cb, this);
        }
    }

    void WlanPage::on_scan_ap_infos(const boost::json::array &ap_infos)
    {
        std::vector<WifiScanApInfo> parsed_ap_infos;
        if (!BROOKESIA_DESCRIBE_FROM_JSON(ap_infos, parsed_ap_infos)) {
            ESP_UTILS_LOGW("Parse Wi-Fi scan result failed");
            return;
        }

        {
            std::lock_guard<std::mutex> lock(ap_info_mutex);
            ap_info = std::move(parsed_ap_infos);
        }
        // One completed round is enough; further scans are requested with Refresh.
        scan_running = false;
        scan_failed = false;
        WifiHelper::call_function_async(WifiHelper::FunctionId::TriggerScanStop);

        if (page_active) {
            lv_async_call([](void *param) {
                auto page = static_cast<WlanPage *>(param);
                if (!page || !page->page_active || !page->list1) {
                    return;
                }
                page->rebuild_network_list();
            }, this);
        }
    }

    void WlanPage::refresh_wifi_state()
    {
        WifiHelper::call_function_async(
            WifiHelper::FunctionId::GetGeneralState,
            [this](service::FunctionResult &&result) {
                if (!page_active || Wifi_state == WIFICLOSE) {
                    return;
                }
                if (!result.success || !result.has_data()) {
                    ESP_UTILS_LOGW("Query Wi-Fi state failed");
                    if (Wifi_state == WIFIOPEN) Wifi_state = DISCONNECT;
                    scan_failed = true;
                    lv_async_call(scan_ui_cb, this);
                    return;
                }
                const auto *state = std::get_if<std::string>(&result.data.value());
                if (!state) {
                    return;
                }
                if (*state == "Connected") {
                    auto_connect_waiting = false;
                    Wifi_state = CONNECTED;
                    scan_running = false;
                    WifiHelper::call_function_async(WifiHelper::FunctionId::TriggerScanStop);
                    refresh_connected_ap();
                } else if (*state == "Started") {
                    check_auto_connect_before_scan();
                } else if (*state == "Connecting") {
                    if (Wifi_state == CONNECTED) return;
                    auto_connect_waiting = false;
                    Wifi_state = CONNECTING;
                    refresh_connected_ap();
                    if (page_active) lv_async_call(scan_ui_cb, this);
                }
            });
    }

    void WlanPage::handle_startup_ap(const std::string &ssid)
    {
        auto_connect_check_pending = false;
        if (!page_active || Wifi_state == WIFICLOSE || Wifi_state == CONNECTED ||
            Wifi_state == CONNECTING) return;
        if (!ssid.empty()) {
            {
                std::lock_guard<std::mutex> lock(wifi_ssid_mutex);
                wifi_ssid = ssid;
            }
            auto_connect_waiting = true;
            Wifi_state = WIFISCAN;
            lv_async_call(scan_ui_cb, this);
            return;
        }
        auto_connect_waiting = false;
        Wifi_state = WIFISCAN;
        start_wifi_scan();
    }

    void WlanPage::check_auto_connect_before_scan()
    {
        if (!page_active || Wifi_state == WIFICLOSE || Wifi_state == CONNECTED ||
            Wifi_state == CONNECTING || auto_connect_check_pending.exchange(true)) return;
        auto query_failed = [this]() {
            auto_connect_check_pending = false;
            ESP_UTILS_LOGW("Query saved Wi-Fi APs failed; skip automatic scan");
            if (Wifi_state == WIFIOPEN) Wifi_state = DISCONNECT;
            if (page_active) lv_async_call(scan_ui_cb, this);
        };
        const bool submitted = WifiHelper::call_function_async(
            WifiHelper::FunctionId::GetConnectAp,
            [this, query_failed](service::FunctionResult &&result) {
                if (!page_active || Wifi_state == WIFICLOSE) {
                    auto_connect_check_pending = false;
                    return;
                }
                if (result.success && result.has_data()) {
                    const auto *json = std::get_if<boost::json::object>(&result.data.value());
                    WifiHelper::ConnectApInfo target;
                    if (json && BROOKESIA_DESCRIBE_FROM_JSON(*json, target) &&
                        !target.ssid.empty()) {
                        handle_startup_ap(target.ssid);
                        return;
                    }
                }
                // Auto-connect also falls back to the last successfully connected AP.
                const bool queried = WifiHelper::call_function_async(
                    WifiHelper::FunctionId::GetConnectedAps,
                    [this, query_failed](service::FunctionResult &&history_result) {
                        if (!history_result.success || !history_result.has_data()) {
                            query_failed();
                            return;
                        }
                        const auto *json = std::get_if<boost::json::array>(&history_result.data.value());
                        std::vector<WifiHelper::ConnectApInfo> history;
                        if (!json || !BROOKESIA_DESCRIBE_FROM_JSON(*json, history)) {
                            query_failed();
                            return;
                        }
                        std::string saved_ssid;
                        for (auto it = history.rbegin(); it != history.rend(); ++it) {
                            if (!it->ssid.empty()) {
                                saved_ssid = it->ssid;
                                break;
                            }
                        }
                        handle_startup_ap(saved_ssid);
                    });
                if (!queried) query_failed();
            });
        if (!submitted) query_failed();
    }

    void WlanPage::start_wifi_scan(bool manual)
    {
        if (!page_active || Wifi_state == WIFICLOSE || Wifi_state == WIFIOPEN ||
            ((Wifi_state == CONNECTED || auto_connect_waiting) && !manual) || Wifi_state == CONNECTING ||
            scan_running || scan_start_pending.exchange(true)) return;
        scan_failed = false;
        if (manual) auto_connect_waiting = false;
        lv_async_call(scan_ui_cb, this);
        const WifiHelper::ScanParams params = {
            .ap_count = 20,
            .interval_ms = 10000,
            .timeout_ms = 60000,
        };
        auto failed = [this]() {
            scan_start_pending = false;
            scan_running = false;
            scan_failed = true;
            if (page_active) lv_async_call(scan_ui_cb, this);
        };
        const bool submitted = WifiHelper::call_function_async(
            WifiHelper::FunctionId::SetScanParams,
            BROOKESIA_DESCRIBE_TO_JSON(params).as_object(),
            [this, manual, failed](service::FunctionResult &&result) {
                if (!result.success) { failed(); return; }
                if (!page_active || Wifi_state == WIFICLOSE || Wifi_state == CONNECTING ||
                    ((Wifi_state == CONNECTED || auto_connect_waiting) && !manual)) {
                    scan_start_pending = false;
                    if (page_active) lv_async_call(scan_ui_cb, this);
                    return;
                }
                const bool started = WifiHelper::call_function_async(
                    WifiHelper::FunctionId::TriggerScanStart,
                    [this, failed](service::FunctionResult &&scan_result) {
                        if (!scan_result.success) { failed(); return; }
                        scan_start_pending = false;
                        if (page_active) lv_async_call(scan_ui_cb, this);
                    });
                if (!started) failed();
            });
        if (!submitted) failed();
    }

    void WlanPage::refresh_connected_ap()
    {
        WifiHelper::call_function_async(
            WifiHelper::FunctionId::GetConnectAp,
            [this](service::FunctionResult &&result) {
                if (!result.success || !result.has_data()) {
                    return;
                }
                const auto *ap_json = std::get_if<boost::json::object>(&result.data.value());
                if (!ap_json) {
                    return;
                }
                WifiHelper::ConnectApInfo ap_info;
                if (!BROOKESIA_DESCRIBE_FROM_JSON(*ap_json, ap_info)) {
                    return;
                }
                {
                    std::lock_guard<std::mutex> lock(wifi_ssid_mutex);
                    wifi_ssid = ap_info.ssid;
                }
                if (page_active) {
                    lv_async_call([](void *param) {
                        auto page = static_cast<WlanPage *>(param);
                        if (!page->page_active || !page->list1) {
                            return;
                        }
                        wifi_state_cb(page);
                    }, this);
                }
            });
    }

    void WlanPage::connect_selected_ap()
    {
        const size_t password_length = strlen(wifi_pwd);
        if (selected_ssid.empty() || (selected_ap_is_locked && password_length < 8)) {
            ESP_UTILS_LOGW("Wi-Fi password is missing or too short");
            if (password_title) {
                lv_label_set_text(password_title, "Password must contain at least 8 characters");
            }
            return;
        }

        const std::string ssid = selected_ssid;
        const std::string password = wifi_pwd;
        {
            std::lock_guard<std::mutex> lock(wifi_ssid_mutex);
            wifi_ssid = ssid;
        }
        Wifi_state = CONNECTING;
        auto_connect_waiting = false;
        scan_running = false;
        WifiHelper::call_function_async(WifiHelper::FunctionId::TriggerScanStop);
        password_editing = false;
        wifi_state_cb(this);

        const bool submitted = WifiHelper::call_function_async(
            WifiHelper::FunctionId::SetConnectAp,
            ssid,
            password,
            [this](service::FunctionResult &&result) {
                if (!result.success) {
                    ESP_UTILS_LOGW("Set Wi-Fi target AP failed: %s", result.error_message.c_str());
                    Wifi_state = DISCONNECT;
                    if (page_active) {
                        lv_async_call(wifi_state_cb, this);
                    }
                    return;
                }

                const bool connect_submitted = WifiHelper::call_function_async(
                    WifiHelper::FunctionId::TriggerGeneralAction,
                    BROOKESIA_DESCRIBE_TO_STR(WifiHelper::GeneralAction::Connect),
                    [this](service::FunctionResult &&connect_result) {
                        if (!connect_result.success) {
                            ESP_UTILS_LOGW("Connect to Wi-Fi AP failed: %s", connect_result.error_message.c_str());
                            Wifi_state = DISCONNECT;
                            if (page_active) {
                                lv_async_call(wifi_state_cb, this);
                            }
                        }
                    });
                if (!connect_submitted) {
                    ESP_UTILS_LOGW("Submit Wi-Fi connect request failed");
                    Wifi_state = DISCONNECT;
                    if (page_active) {
                        lv_async_call(wifi_state_cb, this);
                    }
                }
            });
        if (!submitted) {
            ESP_UTILS_LOGW("Submit Wi-Fi target AP failed");
            Wifi_state = DISCONNECT;
            lv_async_call(wifi_state_cb, this);
        }
    }

    // Load or create persisted page settings.
    bool WlanPage::loadNvsParam(void)
    {
        nvs_handle_t nvs_handle;
        esp_err_t err = nvs_open(NVS_STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE("NVS", "Error (%s) opening NVS handle!", esp_err_to_name(err));
            return false;
        }

        for (auto& key_value : _nvs_param_map) {
            int32_t val = key_value.second;
            err = nvs_get_i32(nvs_handle, key_value.first.c_str(), &val);
            if (err == ESP_OK) {
                key_value.second = val;
                ESP_LOGI("NVS", "Load %s: %ld", key_value.first.c_str(), key_value.second);
            } else if (err == ESP_ERR_NVS_NOT_FOUND) {
                err = nvs_set_i32(nvs_handle, key_value.first.c_str(), val);
                if (err != ESP_OK) {
                    ESP_LOGE("NVS", "Error (%s) setting default for %s", esp_err_to_name(err), key_value.first.c_str());
                    nvs_close(nvs_handle);
                    return false;
                }
                ESP_LOGW("NVS", "Key %s not found, set default: %ld", key_value.first.c_str(), val);
            } else {
                ESP_LOGE("NVS", "Error (%s) reading key %s", esp_err_to_name(err), key_value.first.c_str());
                nvs_close(nvs_handle);
                return false;
            }
        }

        err = nvs_commit(nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE("NVS", "Error (%s) committing NVS changes", esp_err_to_name(err));
            nvs_close(nvs_handle);
            return false;
        }

        nvs_close(nvs_handle);
        return true;
    }

    bool WlanPage::setNvsParam(std::string key, int value)
    {
        nvs_handle_t nvs_handle;
        esp_err_t err = nvs_open(NVS_STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE("NVS", "Error (%s) opening NVS handle!", esp_err_to_name(err));
            return false;
        }

        int32_t v = value;
        err = nvs_set_i32(nvs_handle, key.c_str(), v);
        if (err != ESP_OK) {
            ESP_LOGE("NVS", "Error (%s) setting %s", esp_err_to_name(err), key.c_str());
            nvs_close(nvs_handle);
            return false;
        }

        err = nvs_commit(nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE("NVS", "Error (%s) committing NVS changes", esp_err_to_name(err));
            nvs_close(nvs_handle);
            return false;
        }

        ESP_LOGI("NVS", "Set %s = %d", key.c_str(), value);
        nvs_close(nvs_handle);
        return true;
    }
} // namespace esp_brookesia::apps
