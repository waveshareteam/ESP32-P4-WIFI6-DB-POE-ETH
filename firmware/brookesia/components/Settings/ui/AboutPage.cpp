#include "AboutPage.hpp"
#include "SettingsUI.hpp"
#include "esp_app_desc.h"
#include <cstdio>
#include <cstring>
#include "esp_log.h"
#include "sdkconfig.h"
#include "../Settings.hpp"

#define ESP_UTILS_LOG_TAG "BS:AboutPage"
#include "esp_lib_utils.h"

LV_IMG_DECLARE(logo);

namespace {

const char *product_name()
{
    return "ESP32-P4-WIFI6-DB-POE-ETH";
}

const char *firmware_version()
{
    static char version[7] = {};
    if (version[0] != '\0') {
        return version;
    }

    // Use the firmware build date, including when this file is not recompiled.
    const char *date = esp_app_get_description()->date;
    char month_name[4] = {};
    int day = 0;
    int year = 0;
    if (std::sscanf(date, "%3s %d %d", month_name, &day, &year) != 3 ||
        day < 1 || day > 31 || year < 0) {
        return "Unknown";
    }
    static const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    for (int month = 0; month < 12; ++month) {
        if (std::strcmp(month_name, months[month]) == 0) {
            std::snprintf(version, sizeof(version), "%02d%02d%02d", year % 100, month + 1, day);
            return version;
        }
    }
    return "Unknown";
}

} // namespace

namespace esp_brookesia::apps
{
    AboutPage *AboutPage::_instance = nullptr;

    AboutPage *AboutPage::requestInstance(bool use_status_bar, bool use_navigation_bar)
    {
        if (_instance == nullptr)
        {
            _instance = new AboutPage(use_status_bar, use_navigation_bar);
        }
        return _instance;
    }

    AboutPage::AboutPage(bool use_status_bar, bool use_navigation_bar)
        : App("About", nullptr, true, use_status_bar, use_navigation_bar),
          page_root(nullptr), label(nullptr), list1(nullptr)
    {
    }

    AboutPage::~AboutPage()
    {
    }

    bool AboutPage::run()
    {
        ESP_UTILS_LOGD("AboutPage Run");
        lv_obj_clean(lv_scr_act());

        page_root = settings_ui::create_page(lv_scr_act(), Settings::requestInstance()->getContentTopInset());
        settings_ui::create_header(page_root, "About", [](lv_event_t *e) {
            (void)e;
            lv_async_call([](void *param) {
                (void)param;
                Settings::requestInstance()->showRootPage();
            }, nullptr);
        });

        settings_ui::init_list_styles(style_list, style_list_btn, style_list_text, style_list_btn_pressed);
        list1 = settings_ui::create_content_list(page_root);
        lv_obj_add_style(list1, &style_list, LV_PART_MAIN);

        settings_ui::add_section(list1, "Device", style_list_text);
        settings_ui::add_info_row(list1, &logo, "Manufacturer", "Waveshare", style_list_btn);
        settings_ui::add_info_row(list1, nullptr, "Product", product_name(), style_list_btn);

        settings_ui::add_section(list1, "Software", style_list_text);
        settings_ui::add_info_row(list1, nullptr, "Version", firmware_version(), style_list_btn);
        settings_ui::add_info_row(list1, nullptr, "UI Framework", "ESP-Brookesia", style_list_btn);
        return true;
    }

    bool AboutPage::back()
    {
        ESP_UTILS_LOGD("AboutPage Back");
        Settings::requestInstance()->showRootPage();
        return true;
    }

    bool AboutPage::close()
    {
        ESP_UTILS_LOGD("AboutPage Close");
        if (page_root != nullptr)
        {
            lv_obj_del(page_root);
            page_root = nullptr;
            label = nullptr;
            list1 = nullptr;
            settings_ui::reset_list_styles(style_list, style_list_btn, style_list_text, style_list_btn_pressed);
        }
        return true;
    }

} // namespace esp_brookesia::apps
