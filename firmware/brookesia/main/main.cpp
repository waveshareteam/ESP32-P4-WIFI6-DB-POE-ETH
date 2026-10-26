/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <algorithm>
#include <atomic>
#include <string>
#include <iterator>
#include <new>
#include <vector>
#include "esp_lv_adapter_display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "bsp/esp-bsp.h"
#include "esp_brookesia.hpp"
#include "boost/thread.hpp"
#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "Main"
#include "esp_lib_utils.h"

#include "esp_system.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"

#include "bsp_board_extra.h"
#include "SettingsStorage.hpp"

using namespace esp_brookesia;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems::phone;

constexpr bool EXAMPLE_SHOW_MEM_INFO = true;
constexpr bool EXAMPLE_SHOW_MEM_INFO_LOG = false;

/*
 * Apps register themselves with ESP_UTILS_REGISTER_PLUGIN_WITH_CONSTRUCTOR and
 * are installed from the registry. This table only sets the home-screen order
 * and the boot stage shown while each group installs; names are the app names.
 * Registered apps that are not listed here are installed after the last group.
 */
struct BootAppGroup {
    const char *status;
    int progress;
    std::vector<const char *> apps;
};

static const BootAppGroup BOOT_APP_GROUPS[] = {
    {"Loading application registry...", 65, {"DrawPanel", "Squareline"}},
    {"Loading tools...",                74, {"Calculator", "SpecAnalyzer"}},
    {"Loading media...",                82, {"MusicPlayer", "Camera", "VideoPlayer"}},
    {"Loading system applications...",  90, {"Settings", "Ethernet", "AIChats"}},
};


static std::atomic<bool> s_wifi_connected{false};

// Runs in the Wi-Fi service publisher task; only record the state for the status bar timer.
static void wifi_service_status_handler(const adf_event_t *event, void *ctx)
{
    (void)ctx;

    switch (event->event_id) {
    case BSP_EXTRA_WIFI_EVENT_STA_GOT_IP:
        s_wifi_connected.store(true, std::memory_order_relaxed);
        break;
    case BSP_EXTRA_WIFI_EVENT_STA_LOST_IP:
    case BSP_EXTRA_WIFI_EVENT_DISCONNECTED:
        s_wifi_connected.store(false, std::memory_order_relaxed);
        break;
    default:
        break;
    }
}

static void subscribe_wifi_status_events()
{
    esp_err_t ret = bsp_extra_wifi_service_subscribe(BSP_EXTRA_WIFI_EVENT_ANY, wifi_service_status_handler, nullptr);
    if (ret != ESP_OK) {
        ESP_LOGW(ESP_UTILS_LOG_TAG, "Subscribe Wi-Fi service events failed: %s", esp_err_to_name(ret));
    }
}

static bool is_wifi_connected()
{
    return s_wifi_connected.load(std::memory_order_relaxed);
}

namespace {

class BootLoadingUi {
public:
    static constexpr uint32_t READY_HOLD_MS = 300;
    static constexpr uint32_t FADE_OUT_TIME_MS = 240;

    bool create()
    {
        root_ = lv_obj_create(lv_layer_top());
        if (root_ == nullptr) {
            return false;
        }

        spinner_ = lv_spinner_create(root_);
        title_ = lv_label_create(root_);
        phase_ = lv_label_create(root_);
        status_ = lv_label_create(root_);
        progress_ = lv_bar_create(root_);
        if (spinner_ == nullptr || title_ == nullptr || phase_ == nullptr ||
                status_ == nullptr || progress_ == nullptr) {
            destroy();
            return false;
        }

        lv_obj_remove_style_all(root_);
        lv_obj_set_size(root_, lv_pct(100), lv_pct(100));
        lv_obj_align(root_, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(root_, lv_color_hex(0x101417), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(root_, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_set_size(spinner_, 64, 64);
        lv_obj_align(spinner_, LV_ALIGN_CENTER, 0, -110);
        lv_obj_set_style_arc_width(spinner_, 5, LV_PART_MAIN);
        lv_obj_set_style_arc_width(spinner_, 5, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(spinner_, lv_color_hex(0x313B42), LV_PART_MAIN);
        lv_obj_set_style_arc_color(spinner_, lv_color_hex(0x27C1A8), LV_PART_INDICATOR);

        lv_label_set_text(title_, "ESP32-P4-WIFI6-DB-POE-ETH");
        lv_obj_set_style_text_font(title_, &lv_font_montserrat_32, LV_PART_MAIN);
        lv_obj_set_style_text_color(title_, lv_color_hex(0xF7F9FA), LV_PART_MAIN);
        lv_obj_align(title_, LV_ALIGN_CENTER, 0, -36);

        lv_label_set_text(phase_, "LOADING");
        lv_obj_set_style_text_font(phase_, &lv_font_montserrat_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(phase_, lv_color_hex(0xF2B84B), LV_PART_MAIN);
        lv_obj_align(phase_, LV_ALIGN_CENTER, 0, 4);

        lv_obj_set_width(status_, lv_pct(84));
        lv_label_set_long_mode(status_, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_style_text_align(status_, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(status_, &lv_font_montserrat_18, LV_PART_MAIN);
        lv_obj_set_style_text_color(status_, lv_color_hex(0xA8B1B8), LV_PART_MAIN);
        lv_obj_align(status_, LV_ALIGN_CENTER, 0, 44);

        lv_obj_set_size(progress_, lv_pct(52), 7);
        lv_obj_align(progress_, LV_ALIGN_CENTER, 0, 82);
        lv_bar_set_range(progress_, 0, 100);
        lv_obj_set_style_radius(progress_, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_radius(progress_, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(progress_, lv_color_hex(0x2A343B), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(progress_, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(progress_, lv_color_hex(0x27C1A8), LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(progress_, LV_OPA_COVER, LV_PART_INDICATOR);

        setStage("Preparing display...", 8);
        return true;
    }

    /* Caller must hold the LVGL lock. */
    void setStage(const char *status, int progress)
    {
        if (root_ == nullptr || status == nullptr) {
            return;
        }

        if (progress < 0) {
            progress = 0;
        } else if (progress > 100) {
            progress = 100;
        }

        lv_label_set_text(status_, status);
        lv_bar_set_value(progress_, progress, LV_ANIM_OFF);
        lv_obj_move_foreground(root_);
    }

    void showError(const char *status)
    {
        if (root_ == nullptr) {
            return;
        }

        lv_obj_add_flag(spinner_, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(phase_, "STARTUP FAILED");
        lv_obj_set_style_text_color(phase_, lv_color_hex(0xF06A6A), LV_PART_MAIN);
        lv_obj_align(phase_, LV_ALIGN_CENTER, 0, 4);
        lv_label_set_text(status_, status);
        lv_obj_set_style_text_color(status_, lv_color_hex(0xF7F9FA), LV_PART_MAIN);
        lv_bar_set_value(progress_, 100, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(progress_, lv_color_hex(0xF06A6A), LV_PART_INDICATOR);
        lv_obj_move_foreground(root_);
    }

    void finish()
    {
        if (root_ == nullptr) {
            return;
        }

        setStage("Ready", 100);

        lv_anim_t fade;
        lv_anim_init(&fade);
        lv_anim_set_var(&fade, root_);
        lv_anim_set_values(&fade, LV_OPA_COVER, LV_OPA_TRANSP);
        lv_anim_set_time(&fade, FADE_OUT_TIME_MS);
        lv_anim_set_delay(&fade, READY_HOLD_MS);
        lv_anim_set_exec_cb(&fade, [](void *object, int32_t opacity) {
            lv_obj_set_style_opa(static_cast<lv_obj_t *>(object), opacity, LV_PART_MAIN);
        });
        lv_anim_set_completed_cb(&fade, lv_obj_delete_anim_completed_cb);
        lv_anim_start(&fade);

        // The animation owns root_ now and deletes it from the LVGL task.
        clearObjectPointers();
    }

    void attachToTopLayer(lv_display_t *display)
    {
        attachToLayer(display != nullptr ? lv_display_get_layer_top(display) : nullptr);
    }

    void attachToSystemLayer(lv_display_t *display)
    {
        attachToLayer(display != nullptr ? lv_display_get_layer_sys(display) : nullptr);
    }

    void destroy()
    {
        if (root_ != nullptr) {
            lv_obj_delete(root_);
        }
        clearObjectPointers();
    }

private:
    void clearObjectPointers()
    {
        root_ = nullptr;
        spinner_ = nullptr;
        title_ = nullptr;
        phase_ = nullptr;
        status_ = nullptr;
        progress_ = nullptr;
    }

    void attachToLayer(lv_obj_t *parent)
    {
        if (root_ == nullptr || parent == nullptr) {
            return;
        }

        if (lv_obj_get_parent(root_) != parent) {
            lv_obj_set_parent(root_, parent);
            lv_obj_set_size(root_, lv_pct(100), lv_pct(100));
            lv_obj_align(root_, LV_ALIGN_CENTER, 0, 0);
        }
        lv_obj_move_foreground(root_);
    }

    lv_obj_t *root_ = nullptr;
    lv_obj_t *spinner_ = nullptr;
    lv_obj_t *title_ = nullptr;
    lv_obj_t *phase_ = nullptr;
    lv_obj_t *status_ = nullptr;
    lv_obj_t *progress_ = nullptr;
};

constexpr uint32_t BOOT_FRAME_WAIT_MS = 100;
// The first frame redraws the whole 800x1280 screen in software, which can take
// well over BOOT_FRAME_WAIT_MS.
constexpr uint32_t BOOT_FIRST_FRAME_WAIT_MS = 1000;
// REFR_READY only means LVGL finished drawing. The DPI panel switches to the new
// frame buffer at the next vsync, so allow a couple of panel frames to scan out.
constexpr uint32_t BOOT_SCANOUT_WAIT_MS = 40;
constexpr uint32_t BOOT_BACKLIGHT_FADE_MS = 300;
constexpr uint32_t BOOT_BACKLIGHT_FADE_STEP_MS = 15;

static void on_boot_frame_rendered(lv_event_t *event)
{
    xSemaphoreGive(static_cast<SemaphoreHandle_t>(lv_event_get_user_data(event)));
}

/*
 * Apply an LVGL change and wait until the LVGL task has rendered a frame with
 * it. esp_lv_adapter_refresh_now() must not be used here: with partial tear
 * avoidance its flush waits for a buffer-free notification that the panel ISR
 * sends to the LVGL task, so calling it from app_main deadlocks once all frame
 * buffers are in flight.
 */
template <typename Change>
static bool apply_and_wait_for_frame(lv_display_t *display, Change change,
                                     uint32_t timeout_ms = BOOT_FRAME_WAIT_MS)
{
    static SemaphoreHandle_t frame_rendered = xSemaphoreCreateBinary();

    {
        LvLockGuard gui_guard;
        change();
        if (display == nullptr || frame_rendered == nullptr) {
            return false;
        }
        (void)xSemaphoreTake(frame_rendered, 0);
        // Registered under the same lock as the change, so the next refresh includes it.
        lv_display_add_event_cb(display, on_boot_frame_rendered, LV_EVENT_REFR_READY, frame_rendered);
    }

    const bool rendered = xSemaphoreTake(frame_rendered, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
    if (!rendered) {
        ESP_LOGW(ESP_UTILS_LOG_TAG, "Wait for boot loading frame timed out (%lu ms)",
                 static_cast<unsigned long>(timeout_ms));
    }

    LvLockGuard gui_guard;
    lv_display_remove_event_cb_with_user_data(display, on_boot_frame_rendered, frame_rendered);
    return rendered;
}

/*
 * Ramp the backlight up to target_percent instead of switching it on at once,
 * so power-on does not flash. Quadratic easing compensates for the eye's
 * non-linear response to brightness, keeping the ramp visually even.
 */
static void fade_in_backlight(int target_percent)
{
    const uint32_t steps = BOOT_BACKLIGHT_FADE_MS / BOOT_BACKLIGHT_FADE_STEP_MS;
    for (uint32_t step = 1; step < steps; ++step) {
        const int percent = static_cast<int>(target_percent * step * step / (steps * steps));
        if (bsp_display_brightness_set(percent) != ESP_OK) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(BOOT_BACKLIGHT_FADE_STEP_MS));
    }
    // Always end exactly at the target, even if a step above failed.
    (void)bsp_display_brightness_set(target_percent);
}

/*
 * Draw the stage before the caller starts its work. Most stages hold the LVGL
 * lock while they run, so the LVGL task would otherwise only render the stage
 * after it has already finished.
 */
static void refresh_boot_loading(BootLoadingUi &loading, lv_display_t *display, const char *status, int progress)
{
    ESP_LOGI(ESP_UTILS_LOG_TAG, "Startup stage: %s (%d%%)", status, progress);
    apply_and_wait_for_frame(display, [&]() {
        loading.setStage(status, progress);
    });
}

class BootLoadingFailureGuard {
public:
    BootLoadingFailureGuard(BootLoadingUi &loading, lv_display_t *display)
        : loading_(loading), display_(display)
    {
    }

    ~BootLoadingFailureGuard()
    {
        if (!armed_) {
            return;
        }

        {
            LvLockGuard gui_guard;
            loading_.attachToSystemLayer(display_);
            loading_.showError("Unable to finish startup. Please restart the device.");
        }
        // Startup can fail before the backlight is faded in; make the error visible.
        if (bsp_display_brightness_get() == 0) {
            (void)bsp_display_backlight_on();
        }
    }

    void dismiss()
    {
        armed_ = false;
    }

private:
    BootLoadingUi &loading_;
    lv_display_t *display_;
    bool armed_ = true;
};

class PhoneStartupOwner {
public:
    PhoneStartupOwner(Phone *phone, BootLoadingUi &loading, lv_display_t *display)
        : phone_(phone), loading_(loading), display_(display)
    {
    }

    ~PhoneStartupOwner()
    {
        reset();
    }

    PhoneStartupOwner(const PhoneStartupOwner &) = delete;
    PhoneStartupOwner &operator=(const PhoneStartupOwner &) = delete;

    Phone *get() const
    {
        return phone_;
    }

    Phone *release()
    {
        Phone *phone = phone_;
        phone_ = nullptr;
        return phone;
    }

private:
    void reset()
    {
        if (phone_ == nullptr) {
            return;
        }

        if (!bsp_extra_display_lock(-1)) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Lock display while cleaning up phone failed");
            return;
        }
        loading_.attachToTopLayer(display_);
        delete phone_;
        phone_ = nullptr;
        bsp_extra_display_unlock();
    }

    Phone *phone_;
    BootLoadingUi &loading_;
    lv_display_t *display_;
};

} // namespace



/* Batch-test diagnostic: log only abnormal panel content (a blue flash), so a
 * reproduction leaves evidence in the serial log. Set to false for release. */
constexpr bool DISPLAY_ANOMALY_PROBE = false;
constexpr uint32_t PROBE_PERIOD_MS = 60;

namespace {
void probeSample(const void *fb, uint32_t *avg, uint32_t *blue)
{
    const uint16_t *px = static_cast<const uint16_t *>(fb);
    uint32_t r = 0, g = 0, b = 0, n = 0, blue_cnt = 0;
    *avg = 0; *blue = 0;
    if (px == nullptr) return;
    for (uint32_t y = 0; y < BSP_LCD_V_RES; y += 16) {
        const uint16_t *row = px + (size_t)y * BSP_LCD_H_RES;
        for (uint32_t x = 0; x < BSP_LCD_H_RES; x += 8) {
            const uint16_t v = row[x];
            const uint32_t rr = (v >> 11) & 0x1F, gg = (v >> 5) & 0x3F, bb = v & 0x1F;
            r += rr; g += gg; b += bb; n++;
            if ((bb > 24) && (bb > rr + 10) && (bb > (gg >> 1) + 10)) blue_cnt++;
        }
    }
    if (n) {
        *avg = ((r / n) << 16) | ((g / n) << 8) | (b / n);
        *blue = blue_cnt * 100 / n;
    }
}

void startDisplayAnomalyProbe(void)
{
    static void *fbs[CONFIG_BSP_LCD_DPI_BUFFER_NUMS] = {};
    static uint32_t last_avg[CONFIG_BSP_LCD_DPI_BUFFER_NUMS] = {};
    static uint32_t last_blue[CONFIG_BSP_LCD_DPI_BUFFER_NUMS] = {};
    esp_err_t ret = ESP_OK;
    esp_lcd_panel_handle_t panel = bsp_display_get_panel_handle();
    if (panel == nullptr) return;
#if CONFIG_BSP_LCD_DPI_BUFFER_NUMS >= 3
    ret = esp_lcd_dpi_panel_get_frame_buffer(panel, 3, &fbs[0], &fbs[1], &fbs[2]);
#else
    ret = esp_lcd_dpi_panel_get_frame_buffer(panel, 2, &fbs[0], &fbs[1]);
#endif
    if (ret != ESP_OK) { ESP_LOGE("DispProbe", "no frame buffers"); return; }
    ESP_LOGI("DispProbe", "watching %d frame buffers", CONFIG_BSP_LCD_DPI_BUFFER_NUMS);
    lv_timer_create([](lv_timer_t *) {
        for (size_t i = 0; i < CONFIG_BSP_LCD_DPI_BUFFER_NUMS; ++i) {
            uint32_t avg = 0, blue = 0;
            probeSample(fbs[i], &avg, &blue);
            const bool blue_jump = (blue >= 12) && (last_blue[i] < 12);
            const uint32_t dr = (avg >> 16) & 0xFF, lr = (last_avg[i] >> 16) & 0xFF;
            const bool colour_jump = (dr > lr + 10) || (lr > dr + 10);
            if (blue_jump || (colour_jump && blue >= 8)) {
                ESP_LOGW("DispProbe", "ANOMALY fb%u avg=%06x blue=%u%% (prev %06x/%u%%)",
                         (unsigned)i, (unsigned)avg, (unsigned)blue,
                         (unsigned)last_avg[i], (unsigned)last_blue[i]);
            }
            last_avg[i] = avg; last_blue[i] = blue;
        }
    }, PROBE_PERIOD_MS, nullptr);
}
} // namespace

extern "C" void app_main(void)
{
    esp_log_level_set("rpc_rsp", ESP_LOG_ERROR);

    ESP_UTILS_LOGI("Reset reason: %d", static_cast<int>(esp_reset_reason()));
    ESP_UTILS_LOGI("Display ESP-Brookesia phone demo");

    bsp_display_cfg_t cfg = {
        .lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation = ESP_LV_ADAPTER_ROTATE_0,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL,
        .touch_flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0
        }
    };

    // Keep LVGL on core 1 with a larger PSRAM-backed stack; high-resolution UI and
    // app switching can otherwise starve the GUI task or overflow the default stack.
    cfg.lv_adapter_cfg.task_stack_size = 16 * 1024;
    cfg.lv_adapter_cfg.task_priority = 10;
    cfg.lv_adapter_cfg.task_core_id = 1;
    cfg.lv_adapter_cfg.stack_in_psram = true;

    /* Configure display */
    lv_display_t *disp = bsp_display_start_with_config(&cfg);
    ESP_UTILS_CHECK_NULL_EXIT(disp, "Start display failed");
    bsp_display_backlight_off();

    /* Configure GUI lock */
    LvLock::registerCallbacks([](int timeout_ms) {
        bool locked = bsp_extra_display_lock(timeout_ms);
        ESP_UTILS_CHECK_FALSE_RETURN(locked, false, "Lock failed (timeout_ms: %d)", timeout_ms);

        return true;
    }, []() {
        bsp_extra_display_unlock();
        return true;
    });

    BootLoadingUi boot_loading;
    ESP_LOGI(ESP_UTILS_LOG_TAG, "Startup stage: Preparing display... (8%%)");
    // The backlight stays off until the saved brightness has been loaded below.
    apply_and_wait_for_frame(disp, [&]() {
        if (!boot_loading.create()) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Create boot loading UI failed");
        }
    }, BOOT_FIRST_FRAME_WAIT_MS);

    BootLoadingFailureGuard boot_failure_guard(boot_loading, disp);

    refresh_boot_loading(boot_loading, disp, "Initializing settings...", 16);
    ESP_LOGI(ESP_UTILS_LOG_TAG, "NVS initialization begin");
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
    ESP_LOGI(ESP_UTILS_LOG_TAG, "NVS initialization complete");

    // Load the persisted parameters before any app reads them. Apps can still
    // run without the service, so a failure only disables persistence.
    if (!services::StorageNVS::requestInstance().begin()) {
        ESP_LOGW(ESP_UTILS_LOG_TAG, "StorageNVS service failed to start; settings will not persist");
    }

    // The "Initializing settings..." frame was rendered above; let the panel
    // scan it out, then light the backlight at the user's saved brightness.
    vTaskDelay(pdMS_TO_TICKS(BOOT_SCANOUT_WAIT_MS));
    fade_in_backlight(apps::settings_storage::loadBrightness());

    refresh_boot_loading(boot_loading, disp, "Mounting storage...", 20);
    err = bsp_spiffs_mount();
    if (err != ESP_OK) {
        ESP_LOGE(ESP_UTILS_LOG_TAG, "SPIFFS mount failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(ESP_UTILS_LOG_TAG, "SPIFFS mount successfully");

    refresh_boot_loading(boot_loading, disp, "Starting audio...", 28);
    err = bsp_extra_codec_init();
    if (err != ESP_OK) {
        ESP_LOGE(ESP_UTILS_LOG_TAG, "Codec initialization failed: %s", esp_err_to_name(err));
        return;
    }

    refresh_boot_loading(boot_loading, disp, "Configuring audio...", 45);
    err = bsp_extra_codec_volume_set(apps::settings_storage::loadVolume(), NULL);
    if (err != ESP_OK) {
        ESP_LOGE(ESP_UTILS_LOG_TAG, "Codec volume setup failed: %s", esp_err_to_name(err));
        return;
    }

    /* Create a phone object */
    refresh_boot_loading(boot_loading, disp, "Building home screen...", 55);
    PhoneStartupOwner phone_owner(new (std::nothrow) Phone(disp), boot_loading, disp);
    Phone *phone = phone_owner.get();
    ESP_UTILS_CHECK_NULL_EXIT(phone, "Create phone failed");

    {
        // When operating on non-GUI tasks, should acquire a lock before operating on LVGL
        LvLockGuard gui_guard;

        // Pick the phone stylesheet that matches the panel selected in the BSP.
#if CONFIG_BSP_LCD_TYPE_720_1280_5_INCH_A || CONFIG_BSP_LCD_TYPE_720_1280_7_INCH_A
        const auto &target_stylesheet = ESP_BROOKESIA_PHONE_720_1280_DARK_STYLESHEET();
#elif CONFIG_BSP_LCD_TYPE_800_1280_8_INCH_A || CONFIG_BSP_LCD_TYPE_800_1280_10_1_INCH_A
        const auto &target_stylesheet = ESP_BROOKESIA_PHONE_800_1280_DARK_STYLESHEET();
#else
#error "No Brookesia phone stylesheet for the selected BSP LCD type"
#endif
        ESP_UTILS_CHECK_FALSE_EXIT(phone->addStylesheet(target_stylesheet), "Add display stylesheet failed");
        ESP_UTILS_CHECK_FALSE_EXIT(phone->activateStylesheet(target_stylesheet), "Activate display stylesheet failed");

        /* Begin the phone */
        ESP_UTILS_CHECK_FALSE_EXIT(phone->begin(), "Begin failed");
        boot_loading.attachToSystemLayer(disp);
        // assert(phone->getDisplay().showContainerBorder() && "Show container border failed");
    }

    /* Install registered apps group by group, in BOOT_APP_GROUPS order. */
    std::vector<systems::base::Manager::RegistryAppInfo> registry_apps;
    auto &app_manager = phone->getManager();
    const size_t group_count = std::size(BOOT_APP_GROUPS);
    for (size_t group_index = 0; group_index < group_count; ++group_index) {
        const BootAppGroup &group = BOOT_APP_GROUPS[group_index];
        refresh_boot_loading(boot_loading, disp, group.status, group.progress);

        // Creating and installing apps builds home-screen widgets; hold the LVGL lock.
        LvLockGuard gui_guard;
        if (group_index == 0) {
            ESP_UTILS_CHECK_FALSE_EXIT(app_manager.initAppFromRegistry(registry_apps), "Init app registry failed");
        }

        auto install = [&](const std::string &name, systems::base::App *app) {
            ESP_UTILS_LOGI("Install app: %s", name.c_str());
            return app_manager.checkAppID_Valid(app_manager.installApp(app));
        };
        for (const char *app_name : group.apps) {
            auto it = std::find_if(registry_apps.begin(), registry_apps.end(), [app_name](const auto &info) {
                return std::get<0>(info) == app_name;
            });
            if (it == registry_apps.end()) {
                ESP_UTILS_LOGW("App %s is not registered; skip it", app_name);
                continue;
            }
            ESP_UTILS_CHECK_FALSE_EXIT(install(std::get<0>(*it), std::get<1>(*it).get()),
                                       "Install app %s failed", app_name);
            registry_apps.erase(it);
        }

        if (group_index + 1 == group_count) {
            for (const auto &[name, app] : registry_apps) {
                ESP_UTILS_CHECK_FALSE_EXIT(install(name, app.get()), "Install app %s failed", name.c_str());
            }
            registry_apps.clear();
        }
    }

    // Apps subscribed to Wi-Fi service events during install; start Wi-Fi last so it does not delay the UI.
    refresh_boot_loading(boot_loading, disp, "Starting Wi-Fi...", 93);
    subscribe_wifi_status_events();
    err = bsp_extra_wifi_service_init();
    if (err != ESP_OK) {
        // Keep booting without networking.
        ESP_LOGE(ESP_UTILS_LOG_TAG, "Wi-Fi service initialization failed: %s", esp_err_to_name(err));
    }
    // The service may already have an address (e.g. the init call was a no-op).
    if (bsp_extra_wifi_service_is_connected()) {
        s_wifi_connected.store(true, std::memory_order_relaxed);
    }

    refresh_boot_loading(boot_loading, disp, "Finishing setup...", 96);

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
            StatusBar::WifiState wifi_state = connected ?
                StatusBar::WifiState::SIGNAL_3 :
                StatusBar::WifiState::DISCONNECTED;

            ESP_UTILS_CHECK_FALSE_EXIT(
                phone->getDisplay().getStatusBar()->setWifiIconState(wifi_state),
                "Refresh status bar failed"
            );
        }, 1000, phone);
        ESP_UTILS_CHECK_NULL_EXIT(status_timer, "Create status timer failed");

        phone_owner.release();
        boot_failure_guard.dismiss();
        // Keep the completed state visible briefly, then reveal the home screen smoothly.
        boot_loading.finish();
    }

    if constexpr (DISPLAY_ANOMALY_PROBE) {
        startDisplayAnomalyProbe();
    }

    if constexpr (EXAMPLE_SHOW_MEM_INFO) {
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
                if (EXAMPLE_SHOW_MEM_INFO_LOG) {
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
                }
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
