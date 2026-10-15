#pragma once

#include "esp_brookesia.hpp"
#include "lvgl.h"
#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <string.h>
#include "brookesia/service_helper/network/wifi.hpp"
#include "brookesia/service_manager.hpp"

namespace esp_brookesia::apps {
    class Settings;
}

namespace esp_brookesia::apps {

class WlanPage : public systems::phone::App {
public:
    using WifiHelper = service::helper::Wifi;
    using WifiScanApInfo = WifiHelper::ScanApInfo;

    enum WifiState {
        WIFIOPEN,
        WIFICLOSE,
        WIFISCAN,
        CONNECTED,
        CONNECTING,
        DISCONNECT,
    };


    static WlanPage *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);

    WlanPage(bool use_status_bar, bool use_navigation_bar);
    virtual ~WlanPage();

    bool run() override;
    bool back() override;
    bool close() override;

    void toggleWifiUI(WifiState visible);

private:

    typedef struct {
        WifiScanApInfo ap;
        void *self;
    } btn_user_data_t;

    // Maximum number of APs displayed from a scan.
    #define DEFAULT_SCAN_LIST_SIZE 20

    static WlanPage *_instance;
    int wifi_status_icon_state = 0;

    lv_obj_t *page_root;
    lv_obj_t *label;
    lv_obj_t *list1;
    lv_obj_t *status_btn;
    lv_obj_t *wlan_switch;
    lv_obj_t *wifi_icon;

    lv_obj_t *spinner;
    lv_obj_t *scan_status = nullptr;
    lv_obj_t *refresh_btn = nullptr;
    lv_obj_t *empty_text = nullptr;
    lv_obj_t *connection_spinner = nullptr;
    lv_obj_t *password_title;
    static lv_obj_t *ta;
    static lv_obj_t *kb;
    

    // UI controls that are updated from scan/connect callbacks.
    lv_obj_t *connected_text;
    lv_obj_t *conn_btn;
    lv_obj_t *available_text;
    std::vector<lv_obj_t *> wifi_btns;
    struct ApRow {
        lv_obj_t *button;
        lv_obj_t *progress;
        lv_obj_t *status;
        std::string ssid;
    };
    std::vector<ApRow> ap_rows;

    // Styles are owned by the page and reset when the page closes.
    lv_style_t style_list;
    lv_style_t style_list_btn;
    lv_style_t style_list_text;
    lv_style_t style_list_btn_pressed;

    // Wi-Fi state and service event subscriptions.
    std::atomic_bool page_active{false};
    std::atomic<WifiState> Wifi_state{WIFICLOSE};
    service::EventRegistry::SignalConnection general_event_connection;
    service::EventRegistry::SignalConnection action_event_connection;
    service::EventRegistry::SignalConnection scan_event_connection;
    service::EventRegistry::SignalConnection scan_state_connection;
    std::mutex ap_info_mutex;
    std::mutex wifi_ssid_mutex;
    std::vector<WifiScanApInfo> ap_info;
    std::string wifi_ssid;
    std::string selected_ssid;
    bool selected_ap_is_locked = true;
    std::atomic_bool scan_start_pending{false};
    std::atomic_bool scan_running{false};
    std::atomic_bool scan_failed{false};
    std::atomic_bool auto_connect_waiting{false};
    std::atomic_bool auto_connect_check_pending{false};
    bool password_editing = false;
    char wifi_pwd[65] = {};

    void CreateWifiUI();
    void rebuild_network_list();
    void update_network_ui();
    static void scan_ui_cb(void *param);
    static void wifi_btn_cb(lv_event_t * e);

    static void wifi_state_cb(void *param);
    void subscribe_wifi_events();
    void unsubscribe_wifi_events();
    void on_general_event(const std::string &event, bool is_unexpected);
    void on_scan_ap_infos(const boost::json::array &ap_infos);
    void start_wifi_scan(bool manual = false);
    void refresh_wifi_state();
    void check_auto_connect_before_scan();
    void handle_startup_ap(const std::string &ssid);
    void refresh_connected_ap();
    void connect_selected_ap();
    
    bool OpenWifi();
    bool CloseWifi();

    // Password input and keyboard helpers.
    static void adjust_content_for_keyboard();
    static void restore_content_padding();
    static void kb_event_cb(lv_event_t *e);
    static void ta_event_cb(lv_event_t *e);
    static void connection_event_cb(lv_event_t * e);
    static void cancel_event_cb(lv_event_t * e);
    static lv_obj_t* create_labeled_ta(lv_obj_t *parent, const char *label_txt, const char *placeholder);

    // NVS keys for page-level settings.
    #define NVS_STORAGE_NAMESPACE           "storage"
    #define NVS_KEY_WIFI_ENABLE             "wifi_en"

    std::map<std::string, int32_t> _nvs_param_map;

    bool loadNvsParam(void);
    bool setNvsParam(std::string key, int value);
};

} // namespace esp_brookesia::apps
