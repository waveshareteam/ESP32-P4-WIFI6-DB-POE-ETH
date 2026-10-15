#include "DisplayPage.hpp"
#include "SettingsUI.hpp"
#include <stdint.h>
#include <algorithm>
#include "brookesia/service_helper/system/storage.hpp"
#include "esp_hal_boards/display_device.hpp"
#include "esp_log.h"
#include "../Settings.hpp"

#define ESP_UTILS_LOG_TAG "BS:DisplayPage"
#include "esp_lib_utils.h"

namespace
{
    constexpr char BRIGHTNESS_NAMESPACE[] = "display";
    constexpr char BRIGHTNESS_KEY[] = "brightness";
    constexpr uint32_t STORAGE_TIMEOUT_MS = 1000;
    // Cache the restored / last successfully applied brightness for the UI.
    int s_display_brightness_percent = 100;

    esp_err_t set_display_brightness(int value)
    {
        if (value < 0) {
            value = 0;
        } else if (value > 100) {
            value = 100;
        }

        const esp_err_t ret = esp_hal_boards::display_backlight_set(value);
        if (ret == ESP_OK) {
            s_display_brightness_percent = value;
        }
        return ret;
    }
}

namespace esp_brookesia::apps
{
    DisplayPage *DisplayPage::_instance = nullptr;

    DisplayPage *DisplayPage::requestInstance(bool use_status_bar, bool use_navigation_bar)
    {
        if (_instance == nullptr)
        {
            _instance = new DisplayPage(use_status_bar, use_navigation_bar);
        }
        return _instance;
    }

    DisplayPage::DisplayPage(bool use_status_bar, bool use_navigation_bar)
        : App("Display", nullptr, true, use_status_bar, use_navigation_bar),
          page_root(nullptr), label(nullptr), list1(nullptr)
    {
    }

    DisplayPage::~DisplayPage()
    {
    }

    bool DisplayPage::run()
    {
        ESP_UTILS_LOGD("DisplayPage Run");
        lv_obj_clean(lv_scr_act());

        page_root = settings_ui::create_page(lv_scr_act(), *Settings::requestInstance());
        settings_ui::create_header(page_root, "Display", [](lv_event_t *e) {
            (void)e;
            lv_async_call([](void *param) {
                (void)param;
                Settings::requestInstance()->showRootPage();
            }, nullptr);
        });

        settings_ui::init_list_styles(style_list, style_list_btn, style_list_text, style_list_btn_pressed);
        list1 = settings_ui::create_content_list(page_root);
        lv_obj_add_style(list1, &style_list, LV_PART_MAIN);

        settings_ui::add_section(list1, "Screen", style_list_text);

        lv_obj_t *slider_container = lv_obj_create(list1);
        lv_obj_add_style(slider_container, &style_list_btn, LV_PART_MAIN);
        lv_obj_set_size(slider_container, lv_pct(100), settings_ui::CONTROL_PANEL_HEIGHT);
        lv_obj_set_style_pad_all(slider_container, 20, LV_PART_MAIN);
        lv_obj_clear_flag(slider_container, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(slider_container, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *title = lv_label_create(slider_container);
        lv_label_set_text(title, "Brightness");
        lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
        lv_obj_set_style_text_color(title, lv_color_hex(settings_ui::COLOR_PRIMARY_TEXT), LV_PART_MAIN);
        lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

        s_display_brightness_percent = esp_hal_boards::display_backlight_get_saved();
        int value = s_display_brightness_percent;
        if (value < 0) {
            value = 0;
        } else if (value > 100) {
            value = 100;
        }

        label = lv_label_create(slider_container);
        lv_label_set_text_fmt(label, "%ld%%", static_cast<long>(value));
        lv_obj_set_style_text_font(label, &lv_font_montserrat_22, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_hex(settings_ui::COLOR_ACCENT), LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_TOP_RIGHT, 0, 1);

        lv_obj_t *brightness_slider = lv_slider_create(slider_container);
        lv_obj_set_width(brightness_slider, lv_pct(100));
        lv_obj_set_height(brightness_slider, 14);
        lv_obj_align(brightness_slider, LV_ALIGN_BOTTOM_MID, 0, -5);
        lv_slider_set_range(brightness_slider, 0, 100);
        lv_slider_set_value(brightness_slider, value, LV_ANIM_OFF);
        const esp_err_t apply_ret = set_display_brightness(value);
        if (apply_ret != ESP_OK) {
            ESP_LOGW("DisplayPage", "Restore display brightness failed: %s", esp_err_to_name(apply_ret));
        }

        lv_obj_set_style_bg_color(brightness_slider, lv_color_hex(0x505050), LV_PART_MAIN);
        lv_obj_set_style_bg_color(brightness_slider, lv_color_hex(settings_ui::COLOR_ACCENT), LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(brightness_slider, lv_color_white(), LV_PART_KNOB);
        lv_obj_set_style_pad_all(brightness_slider, 7, LV_PART_KNOB);
        lv_obj_set_style_radius(brightness_slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
        lv_obj_add_event_cb(brightness_slider, event_handler_cb, LV_EVENT_VALUE_CHANGED, this);
        lv_obj_add_event_cb(brightness_slider, event_handler_cb, LV_EVENT_RELEASED, this);
        return true;
    }

    void DisplayPage::event_handler_cb(lv_event_t *e)
    {
        DisplayPage *instance = static_cast<DisplayPage *>(lv_event_get_user_data(e));
        lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
        if (instance == nullptr || slider == nullptr || !lv_obj_has_class(slider, &lv_slider_class)) {
            return;
        }

        const int32_t value = lv_slider_get_value(slider);
        if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
            // Persist only the value successfully applied to the backlight.
            auto result = esp_brookesia::service::helper::Storage::save_key_value(
                              BRIGHTNESS_NAMESPACE, BRIGHTNESS_KEY,
                              static_cast<int32_t>(s_display_brightness_percent), STORAGE_TIMEOUT_MS);
            if (!result) {
                ESP_LOGW("DisplayPage", "Save brightness failed: %s", result.error().c_str());
            }
            return;
        }
        if (instance->label != nullptr) {
            lv_label_set_text_fmt(instance->label, "%ld%%", static_cast<long>(value));
        }
        const esp_err_t ret = set_display_brightness(value);
        if (ret != ESP_OK) {
            ESP_LOGW("DisplayPage", "Set display brightness failed: %s", esp_err_to_name(ret));
        }
    }

    bool DisplayPage::back()
    {
        ESP_UTILS_LOGD("DisplayPage Back");
        Settings::requestInstance()->showRootPage();
        return true;
    }

    bool DisplayPage::close()
    {
        ESP_UTILS_LOGD("DisplayPage Close");
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
