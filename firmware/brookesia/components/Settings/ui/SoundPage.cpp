#include "SoundPage.hpp"
#include "SettingsUI.hpp"
#include "esp_log.h"
#include "../Settings.hpp"
#include <algorithm>
#include <optional>
#include "brookesia/service_helper/media/audio.hpp"
#include "brookesia/service_manager/service/manager.hpp"
#include "brookesia/service_helper/system/storage.hpp"

#define ESP_UTILS_LOG_TAG "BS:SoundPage"
#include "esp_lib_utils.h"

namespace esp_brookesia::apps
{
    namespace {
        using StorageHelper = service::helper::Storage;
        using AudioPlaybackHelper = service::helper::AudioPlayback;
        constexpr char MEDIA_VOLUME_STORAGE_KEY[] = "media_volume";
        constexpr char MEDIA_VOLUME_INITIALIZED_KEY[] = "media_volume_initialized";
        constexpr char MEDIA_VOLUME_STORAGE_NAMESPACE[] = "storage";
        constexpr int DEFAULT_MEDIA_VOLUME = 70;
        constexpr int MAX_MEDIA_VOLUME = 95;
        constexpr uint32_t STORAGE_TIMEOUT_MS = 1000;
        int last_media_volume = DEFAULT_MEDIA_VOLUME;

        bool bind_audio_playback_service(service::ServiceBinding &binding)
        {
            if (binding.is_valid()) {
                return true;
            }

            auto &manager = service::ServiceManager::get_instance();
            if (!manager.is_initialized() && !manager.init()) {
                ESP_LOGW("SoundPage", "Initialize service manager failed");
                return false;
            }
            if (!manager.is_running() && !manager.start()) {
                ESP_LOGW("SoundPage", "Start service manager failed");
                return false;
            }

            binding = manager.bind(AudioPlaybackHelper::get_name().data());
            if (!binding.is_valid()) {
                ESP_LOGW("SoundPage", "Bind audio playback service failed");
                return false;
            }
            return true;
        }

        std::optional<int> read_media_volume()
        {
            auto result = StorageHelper::get_key_value<int32_t>(
                              MEDIA_VOLUME_STORAGE_NAMESPACE, MEDIA_VOLUME_STORAGE_KEY, STORAGE_TIMEOUT_MS);
            if (!result) {
                return std::nullopt;
            }
            return std::clamp(static_cast<int>(result.value()), 0, MAX_MEDIA_VOLUME);
        }

        bool save_media_volume(int volume)
        {
            auto result = StorageHelper::save_key_value(
                              MEDIA_VOLUME_STORAGE_NAMESPACE, MEDIA_VOLUME_STORAGE_KEY,
                              static_cast<int32_t>(std::clamp(volume, 0, MAX_MEDIA_VOLUME)), STORAGE_TIMEOUT_MS);
            if (!result) {
                ESP_LOGW("SoundPage", "Storage save media volume failed: %s", result.error().c_str());
            }
            return result.has_value();
        }

        void mark_media_volume_initialized()
        {
            auto result = StorageHelper::save_key_value(
                              MEDIA_VOLUME_STORAGE_NAMESPACE, MEDIA_VOLUME_INITIALIZED_KEY,
                              static_cast<int32_t>(1), STORAGE_TIMEOUT_MS);
            if (!result) {
                ESP_LOGW("SoundPage", "Save media volume initialization flag failed: %s", result.error().c_str());
            }
        }
    }

    SoundPage *SoundPage::_instance = nullptr;

    SoundPage *SoundPage::requestInstance(bool use_status_bar, bool use_navigation_bar)
    {
        if (_instance == nullptr)
        {
            _instance = new SoundPage(use_status_bar, use_navigation_bar);
        }
        return _instance;
    }

    SoundPage::SoundPage(bool use_status_bar, bool use_navigation_bar)
        : App("Sound", nullptr, true, use_status_bar, use_navigation_bar),
          page_root(nullptr), label(nullptr), list1(nullptr)
    {
    }

    bool SoundPage::run()
    {
        ESP_UTILS_LOGD("SoundPage Run");
        lv_obj_clean(lv_scr_act());

        page_root = settings_ui::create_page(lv_scr_act(), *Settings::requestInstance());
        settings_ui::create_header(page_root, "Sound", [](lv_event_t *e) {
            (void)e;
            lv_async_call([](void *param) {
                (void)param;
                Settings::requestInstance()->showRootPage();
            }, nullptr);
        });

        settings_ui::init_list_styles(style_list, style_list_btn, style_list_text, style_list_btn_pressed);
        list1 = settings_ui::create_content_list(page_root);
        lv_obj_add_style(list1, &style_list, LV_PART_MAIN);

        settings_ui::add_section(list1, "Output", style_list_text);

        lv_obj_t *slider_container = lv_obj_create(list1);
        lv_obj_add_style(slider_container, &style_list_btn, LV_PART_MAIN);
        lv_obj_set_size(slider_container, lv_pct(100), settings_ui::CONTROL_PANEL_HEIGHT);
        lv_obj_set_style_pad_all(slider_container, 20, LV_PART_MAIN);
        lv_obj_clear_flag(slider_container, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(slider_container, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *title = lv_label_create(slider_container);
        lv_label_set_text(title, "Media volume");
        lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
        lv_obj_set_style_text_color(title, lv_color_hex(settings_ui::COLOR_PRIMARY_TEXT), LV_PART_MAIN);
        lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

        const bool audio_service_available = bind_audio_playback_service(audio_service_binding);
        const auto saved_volume = read_media_volume();
        auto initialized_result = StorageHelper::get_key_value<int32_t>(
                                     MEDIA_VOLUME_STORAGE_NAMESPACE, MEDIA_VOLUME_INITIALIZED_KEY,
                                     STORAGE_TIMEOUT_MS);
        const bool volume_was_initialized = initialized_result && initialized_result.value() != 0;
        bool volume_value_saved = saved_volume.has_value();
        if (saved_volume) {
            last_media_volume = saved_volume.value();
            // The previous startup path accidentally persisted the service's
            // initial zero. Migrate that unmarked value to the intended default.
            if (last_media_volume == 0 && !volume_was_initialized) {
                last_media_volume = DEFAULT_MEDIA_VOLUME;
                volume_value_saved = save_media_volume(last_media_volume);
            }
        } else {
            last_media_volume = DEFAULT_MEDIA_VOLUME;
            volume_value_saved = save_media_volume(last_media_volume);
        }
        if (!volume_was_initialized && volume_value_saved) {
            mark_media_volume_initialized();
        }

        if (audio_service_available) {
            auto result = AudioPlaybackHelper::call_function_sync(
                              AudioPlaybackHelper::FunctionId::SetVolume,
                              static_cast<double>(last_media_volume)
                          );
            if (!result) {
                ESP_LOGW("SoundPage", "Restore audio output volume failed: %s", result.error().c_str());
            }
        }
        const uint32_t value = static_cast<uint32_t>(last_media_volume);

        label = lv_label_create(slider_container);
        lv_label_set_text_fmt(label, "%lu%%", static_cast<unsigned long>(value));
        lv_obj_set_style_text_font(label, &lv_font_montserrat_22, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_hex(settings_ui::COLOR_ACCENT), LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_TOP_RIGHT, 0, 1);

        lv_obj_t *volume_slider = lv_slider_create(slider_container);
        lv_obj_set_width(volume_slider, lv_pct(100));
        lv_obj_set_height(volume_slider, 14);
        lv_obj_align(volume_slider, LV_ALIGN_BOTTOM_MID, 0, -5);
        lv_slider_set_range(volume_slider, 0, 95);
        lv_slider_set_value(volume_slider, value, LV_ANIM_OFF);

        lv_obj_set_style_bg_color(volume_slider, lv_color_hex(0x505050), LV_PART_MAIN);
        lv_obj_set_style_bg_color(volume_slider, lv_color_hex(settings_ui::COLOR_ACCENT), LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(volume_slider, lv_color_white(), LV_PART_KNOB);
        lv_obj_set_style_pad_all(volume_slider, 7, LV_PART_KNOB);
        lv_obj_set_style_radius(volume_slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
        lv_obj_add_event_cb(volume_slider, event_handler_cb, LV_EVENT_VALUE_CHANGED, this);
        lv_obj_add_event_cb(volume_slider, event_handler_cb, LV_EVENT_RELEASED, this);
        return true;
    }

    void SoundPage::event_handler_cb(lv_event_t *e)
    {
        SoundPage *instance = static_cast<SoundPage *>(lv_event_get_user_data(e));
        lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
        if (instance == nullptr || slider == nullptr || !lv_obj_has_class(slider, &lv_slider_class)) {
            return;
        }

        const uint32_t value = lv_slider_get_value(slider);
        if (instance->label != nullptr) {
            lv_label_set_text_fmt(instance->label, "%lu%%", static_cast<unsigned long>(value));
        }

        // Keep dragging responsive; apply and persist the volume only after release.
        if (lv_event_get_code(e) != LV_EVENT_RELEASED) {
            return;
        }

        if (!bind_audio_playback_service(instance->audio_service_binding)) {
            lv_slider_set_value(slider, last_media_volume, LV_ANIM_OFF);
            if (instance->label != nullptr) {
                lv_label_set_text_fmt(instance->label, "%d%%", last_media_volume);
            }
            return;
        }
        auto result = AudioPlaybackHelper::call_function_sync(
                          AudioPlaybackHelper::FunctionId::SetVolume,
                          static_cast<double>(value)
                      );
        if (!result) {
            ESP_LOGW("SoundPage", "Set audio output volume failed: %s", result.error().c_str());
            lv_slider_set_value(slider, last_media_volume, LV_ANIM_OFF);
            if (instance->label != nullptr) {
                lv_label_set_text_fmt(instance->label, "%d%%", last_media_volume);
            }
            return;
        }

        last_media_volume = static_cast<int>(value);
        const bool saved = save_media_volume(last_media_volume);
        if (!saved) {
            ESP_LOGW("SoundPage", "Save audio output volume failed");
        } else {
            mark_media_volume_initialized();
        }
    }

    bool SoundPage::back()
    {
        ESP_UTILS_LOGD("SoundPage Back");
        Settings::requestInstance()->showRootPage();
        return true;
    }

    bool SoundPage::close()
    {
        ESP_UTILS_LOGD("SoundPage Close");
        audio_service_binding.release();
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
