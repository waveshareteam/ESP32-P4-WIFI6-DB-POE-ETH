/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "lvgl.h"

#include "gpio_monitor.h"

static const char *TAG = "gpio_monitor";

/* Theme colors */

#define COLOR_BG_SCREEN     0x101820
#define COLOR_BG_HEADER     0x172A3A
#define COLOR_BG_LEVEL_HIGH 0x18794E
#define COLOR_BG_LEVEL_LOW  0x263238
#define COLOR_BORDER        0x607D8B
#define COLOR_TEXT_PRIMARY   0xF5F7FA
#define COLOR_TEXT_SECONDARY 0xB0BEC5

/* Layout constants */

#define MIN_CELL_WIDTH      88
#define MIN_CELL_HEIGHT     42
#define GRID_PADDING        6
#define GRID_GAP            6
#define CARD_RADIUS         8
#define CARD_PAD            4

/* Internal types */

typedef struct {
    lv_obj_t *card;
    lv_obj_t *label;
    size_t pin_index;
    int last_level;
    bool level_initialized;
    bool passed;
    bool input_checked_latched;
} gpio_monitor_pin_view_t;

static struct {
    gpio_monitor_pin_config_t *pins;
    gpio_monitor_pin_view_t *views;
    size_t pin_count;
    uint32_t refresh_period_ms;
    lv_timer_t *refresh_timer;
    lv_obj_t *count_label;
    lv_obj_t *mode_label;
#ifdef DYNAMIC_SWITCH_GPIO_MODE
    lv_obj_t *mode_switch;
#endif
    lv_obj_t *passed_card;
    lv_obj_t *passed_label;
    size_t passed_count;
    bool output_enabled;
    bool initialized;
    bool ui_started;
} s_monitor;

/* Helpers */

static gpio_mode_t current_gpio_mode(void)
{
    // INPUT_OUTPUT keeps the input path active so gpio_get_level() can read back the output level.
    return s_monitor.output_enabled ? GPIO_MODE_INPUT_OUTPUT : GPIO_MODE_INPUT;
}

static const char *gpio_mode_str(gpio_mode_t mode)
{
    return (mode & GPIO_MODE_DEF_OUTPUT) ? "OUT" : "IN";
}

static lv_color_t level_color(int level)
{
    return lv_color_hex(level ? COLOR_BG_LEVEL_HIGH : COLOR_BG_LEVEL_LOW);
}

static int32_t at_least_one(int32_t value)
{
    return value > 0 ? value : 1;
}

static size_t choose_column_count(size_t pin_count, int32_t width)
{
    size_t max_cols = (size_t)(width / MIN_CELL_WIDTH);
    if (max_cols == 0) {
        max_cols = 1;
    }
    if (max_cols > pin_count) {
        max_cols = pin_count;
    }

    size_t cols = 1;
    while (cols * cols < pin_count && cols < max_cols) {
        cols++;
    }
    return cols;
}

static int find_pin_index(gpio_num_t gpio_num)
{
    for (size_t i = 0; i < s_monitor.pin_count; ++i) {
        if (s_monitor.pins[i].gpio_num == gpio_num) {
            return (int)i;
        }
    }
    return -1;
}

/* Passed-count tracking */

static void update_passed_card(void)
{
    if (s_monitor.passed_label) {
        lv_label_set_text_fmt(s_monitor.passed_label, "Passed: %u", (unsigned)s_monitor.passed_count);
    }
    if (s_monitor.passed_card) {
        bool all_passed = s_monitor.passed_count == s_monitor.pin_count;
        lv_color_t color = lv_color_hex(all_passed ? COLOR_BG_LEVEL_HIGH : COLOR_BG_LEVEL_LOW);
        lv_obj_set_style_bg_color(s_monitor.passed_card, color, 0);
    }
}

static void reset_passed_state(void)
{
    s_monitor.passed_count = 0;
    if (s_monitor.views) {
        for (size_t i = 0; i < s_monitor.pin_count; ++i) {
            s_monitor.views[i].level_initialized = false;
            s_monitor.views[i].passed = false;
            s_monitor.views[i].input_checked_latched = false;
        }
    }
    update_passed_card();
}

/* Per-pin view update */

static void detect_transition(gpio_monitor_pin_view_t *view, int level)
{
    if (!view->level_initialized) {
        view->level_initialized = true;
    } else if (current_gpio_mode() == GPIO_MODE_INPUT && level != view->last_level && !view->passed) {
        view->passed = true;
        s_monitor.passed_count++;
        update_passed_card();
    }
    view->last_level = level;
}

static void render_pin_view(size_t index, int level)
{
    const gpio_monitor_pin_config_t *pin = &s_monitor.pins[index];
    gpio_monitor_pin_view_t *view = &s_monitor.views[index];
    const gpio_mode_t mode = current_gpio_mode();
    const char *mode_str = gpio_mode_str(mode);

    if (pin->name) {
        lv_label_set_text_fmt(view->label, "%s %s:%d", pin->name, mode_str, level);
    } else {
        lv_label_set_text_fmt(view->label, "GPIO%d %s:%d", pin->gpio_num, mode_str, level);
    }

    lv_obj_set_style_bg_color(view->card, level_color(level), 0);

    bool checked = level != 0;
#if GPIO_MONITOR_LATCH_INPUT_HIGH
    if (mode == GPIO_MODE_INPUT && level) {
        view->input_checked_latched = true;
    }
    checked = checked || (mode == GPIO_MODE_INPUT && view->input_checked_latched);
#endif

    if (checked) {
        lv_obj_add_state(view->card, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(view->card, LV_STATE_CHECKED);
    }

    if (s_monitor.output_enabled) {
        lv_obj_add_flag(view->card, LV_OBJ_FLAG_CLICKABLE);
    } else {
        lv_obj_clear_flag(view->card, LV_OBJ_FLAG_CLICKABLE);
    }
}

static void update_pin_view(size_t index, bool track_transition)
{
    gpio_monitor_pin_view_t *view = &s_monitor.views[index];
    int level = gpio_get_level(s_monitor.pins[index].gpio_num);

    if (track_transition) {
        /* Skip the LVGL re-render when the level hasn't changed since the last
         * periodic tick; re-formatting the label and re-styling the card for
         * every idle pin every refresh_period_ms is pure wasted CPU. */
        bool level_changed = !view->level_initialized || level != view->last_level;
        detect_transition(view, level);
        if (!level_changed) {
            return;
        }
    } else {
        view->last_level = level;
    }
    render_pin_view(index, level);
}

/* LVGL callbacks */

static void refresh_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    for (size_t i = 0; i < s_monitor.pin_count; ++i) {
        update_pin_view(i, true);
    }
}

static void output_card_event_cb(lv_event_t *event)
{
    gpio_monitor_pin_view_t *view = lv_event_get_user_data(event);
    if (!view || !s_monitor.output_enabled) {
        return;
    }

    size_t i = view->pin_index;
    int cur = gpio_get_level(s_monitor.pins[i].gpio_num);
    esp_err_t ret = gpio_monitor_set_level(s_monitor.pins[i].gpio_num, !cur);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to set GPIO %d level: %s", s_monitor.pins[i].gpio_num, esp_err_to_name(ret));
    } else {
        update_pin_view(i, false);
    }
}

#ifdef DYNAMIC_SWITCH_GPIO_MODE
static void mode_switch_event_cb(lv_event_t *event)
{
    lv_obj_t *sw = lv_event_get_target(event);
    bool want_output = lv_obj_has_state(sw, LV_STATE_CHECKED);
    esp_err_t err = gpio_monitor_set_output_enabled(want_output);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to switch GPIO mode: %s", esp_err_to_name(err));
        if (want_output) {
            lv_obj_clear_state(sw, LV_STATE_CHECKED);
        } else {
            lv_obj_add_state(sw, LV_STATE_CHECKED);
        }
    }
}
#endif

/* GPIO HW helpers */

static esp_err_t validate_config(const gpio_monitor_config_t *config)
{
    if (!config || (config->pin_count > 0 && !config->pins)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (config->initial_mode != GPIO_MODE_INPUT && config->initial_mode != GPIO_MODE_OUTPUT) {
        ESP_LOGE(TAG, "only GPIO_MODE_INPUT and GPIO_MODE_OUTPUT are supported");
        return ESP_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < config->pin_count; ++i) {
        gpio_num_t num = config->pins[i].gpio_num;
        if (num < GPIO_NUM_0 || num >= GPIO_NUM_MAX) {
            ESP_LOGE(TAG, "invalid GPIO configuration at index %u", (unsigned)i);
            return ESP_ERR_INVALID_ARG;
        }
        for (size_t j = 0; j < i; ++j) {
            if (config->pins[j].gpio_num == num) {
                ESP_LOGE(TAG, "GPIO%d is configured more than once", num);
                return ESP_ERR_INVALID_ARG;
            }
        }
    }
    return ESP_OK;
}

static esp_err_t configure_gpio(size_t index)
{
    const gpio_monitor_pin_config_t *pin = &s_monitor.pins[index];
    const gpio_mode_t mode = current_gpio_mode();
    bool is_output = s_monitor.output_enabled;
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << pin->gpio_num,
        .mode = mode,
        .pull_up_en = is_output ? GPIO_PULLUP_DISABLE : pin->pull_up_en,
        .pull_down_en = is_output ? GPIO_PULLDOWN_DISABLE : pin->pull_down_en,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_RETURN_ON_ERROR(gpio_config(&cfg), TAG, "gpio_config GPIO%d failed", pin->gpio_num);
    if (is_output) {
        ESP_RETURN_ON_ERROR(gpio_set_level(pin->gpio_num, pin->initial_level ? 1 : 0),
                            TAG, "gpio_set_level GPIO%d failed", pin->gpio_num);
    }
    return ESP_OK;
}

static esp_err_t configure_all_gpios(void)
{
    for (size_t i = 0; i < s_monitor.pin_count; ++i) {
        ESP_RETURN_ON_ERROR(configure_gpio(i), TAG, "GPIO%d initialization failed", s_monitor.pins[i].gpio_num);
    }
    return ESP_OK;
}

static void reset_all_gpios(void)
{
    for (size_t i = 0; i < s_monitor.pin_count; ++i) {
        esp_err_t err = gpio_reset_pin(s_monitor.pins[i].gpio_num);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "gpio_reset_pin GPIO%d failed: %s", s_monitor.pins[i].gpio_num, esp_err_to_name(err));
        }
    }
}

/* UI construction */

static void style_card(lv_obj_t *card)
{
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(card, CARD_RADIUS, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(COLOR_BORDER), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(card, CARD_PAD, 0);
}

static lv_obj_t *create_header(lv_obj_t *screen, int32_t width, int32_t height)
{
    lv_obj_t *header = lv_obj_create(screen);
    if (!header) {
        return NULL;
    }
    lv_obj_set_size(header, width, height);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(COLOR_BG_HEADER), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(header, CARD_PAD, 0);

    lv_obj_t *title = lv_label_create(header);
    if (!title) {
        return NULL;
    }
    lv_label_set_text(title, "GPIO monitor");
    lv_obj_set_style_text_color(title, lv_color_hex(COLOR_TEXT_PRIMARY), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 4, 0);

    s_monitor.count_label = lv_label_create(header);
    if (!s_monitor.count_label) {
        return NULL;
    }
    lv_label_set_text_fmt(s_monitor.count_label, "Total Nums : %u GPIO", (unsigned)s_monitor.pin_count);
    lv_obj_set_style_text_color(s_monitor.count_label, lv_color_hex(COLOR_TEXT_SECONDARY), 0);
    lv_obj_align(s_monitor.count_label, LV_ALIGN_RIGHT_MID, -116, 0);

#ifdef DYNAMIC_SWITCH_GPIO_MODE
    s_monitor.mode_label = lv_label_create(header);
    if (!s_monitor.mode_label) {
        return NULL;
    }
    lv_label_set_text(s_monitor.mode_label, "IN");
    lv_obj_set_style_text_color(s_monitor.mode_label, lv_color_hex(COLOR_TEXT_PRIMARY), 0);
    lv_obj_align(s_monitor.mode_label, LV_ALIGN_RIGHT_MID, -68, 0);

    s_monitor.mode_switch = lv_switch_create(header);
    if (!s_monitor.mode_switch) {
        return NULL;
    }
    lv_obj_set_size(s_monitor.mode_switch, 52, 28);
    lv_obj_align(s_monitor.mode_switch, LV_ALIGN_RIGHT_MID, -8, 0);
    if (s_monitor.output_enabled) {
        lv_obj_add_state(s_monitor.mode_switch, LV_STATE_CHECKED);
        lv_label_set_text(s_monitor.mode_label, "OUT");
    }
    lv_obj_set_style_bg_color(s_monitor.mode_switch, lv_color_hex(COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_monitor.mode_switch, lv_color_hex(COLOR_BG_LEVEL_HIGH),
                              LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_add_event_cb(s_monitor.mode_switch, mode_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
#endif

    return header;
}

static esp_err_t create_passed_card(lv_obj_t *screen, int32_t screen_width, int32_t y, int32_t height)
{
    s_monitor.passed_card = lv_obj_create(screen);
    if (!s_monitor.passed_card) {
        return ESP_ERR_NO_MEM;
    }
    lv_obj_set_pos(s_monitor.passed_card, GRID_GAP, y);
    lv_obj_set_size(s_monitor.passed_card, at_least_one(screen_width - 2 * GRID_GAP), height);
    style_card(s_monitor.passed_card);
    lv_obj_set_style_bg_color(s_monitor.passed_card, lv_color_hex(COLOR_BG_LEVEL_LOW), 0);

    s_monitor.passed_label = lv_label_create(s_monitor.passed_card);
    if (!s_monitor.passed_label) {
        return ESP_ERR_NO_MEM;
    }
    lv_obj_set_style_text_color(s_monitor.passed_label, lv_color_hex(COLOR_TEXT_PRIMARY), 0);
    lv_obj_center(s_monitor.passed_label);
    update_passed_card();
    return ESP_OK;
}

static lv_obj_t *create_grid_container(lv_obj_t *screen, int32_t screen_width, int32_t y, int32_t height)
{
    lv_obj_t *grid = lv_obj_create(screen);
    if (!grid) {
        return NULL;
    }
    lv_obj_set_pos(grid, 0, y);
    lv_obj_set_size(grid, screen_width, height);
    lv_obj_add_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(grid, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_radius(grid, 0, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_bg_color(grid, lv_color_hex(COLOR_BG_SCREEN), 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    return grid;
}

static esp_err_t create_pin_cards(lv_obj_t *grid, int32_t screen_width,
                                  int32_t inner_width, int32_t inner_height, int32_t grid_height)
{
    const size_t cols = choose_column_count(s_monitor.pin_count, inner_width);
    const size_t rows = (s_monitor.pin_count + cols - 1) / cols;

    int32_t raw_cell_h = at_least_one((inner_height - GRID_GAP * (int32_t)(rows - 1)) / (int32_t)rows);
    const int32_t cell_h = raw_cell_h < MIN_CELL_HEIGHT ? MIN_CELL_HEIGHT : raw_cell_h;
    const int32_t cell_w = at_least_one((inner_width - GRID_GAP * (int32_t)(cols - 1)) / (int32_t)cols);
    const int32_t content_h = at_least_one(GRID_PADDING * 2 + (int32_t)rows * cell_h + GRID_GAP * (int32_t)(rows - 1));

    s_monitor.views = calloc(s_monitor.pin_count, sizeof(*s_monitor.views));
    if (!s_monitor.views) {
        return ESP_ERR_NO_MEM;
    }

    lv_obj_t *content = lv_obj_create(grid);
    if (!content) {
        return ESP_ERR_NO_MEM;
    }
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(content, 0, 0);
    lv_obj_set_size(content, screen_width, content_h > grid_height ? content_h : grid_height);
    lv_obj_set_style_radius(content, 0, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(content, 0, 0);

    ESP_LOGI(TAG, "UI: %ldx%ld, %u GPIO, %u cols x %u rows, card %ldx%ld",
             (long)screen_width, (long)(inner_height + 2 * GRID_PADDING),
             (unsigned)s_monitor.pin_count, (unsigned)cols, (unsigned)rows, (long)cell_w, (long)cell_h);

    for (size_t i = 0; i < s_monitor.pin_count; ++i) {
        gpio_monitor_pin_view_t *view = &s_monitor.views[i];
        view->pin_index = i;

        int32_t col = (int32_t)(i % cols);
        int32_t row = (int32_t)(i / cols);

        view->card = lv_obj_create(content);
        if (!view->card) {
            return ESP_ERR_NO_MEM;
        }
        lv_obj_set_pos(view->card, GRID_PADDING + col * (cell_w + GRID_GAP), GRID_PADDING + row * (cell_h + GRID_GAP));
        lv_obj_set_size(view->card, cell_w, cell_h);
        style_card(view->card);
        lv_obj_add_flag(view->card, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_style_bg_color(view->card, lv_color_hex(COLOR_BG_LEVEL_HIGH), LV_PART_MAIN | LV_STATE_CHECKED);

        view->label = lv_label_create(view->card);
        if (!view->label) {
            return ESP_ERR_NO_MEM;
        }
        lv_obj_set_width(view->label, LV_PCT(100));
        lv_obj_set_style_text_color(view->label, lv_color_hex(COLOR_TEXT_PRIMARY), 0);
        lv_obj_set_style_text_align(view->label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(view->label);
        update_pin_view(i, false);

        lv_obj_add_event_cb(view->card, output_card_event_cb, LV_EVENT_CLICKED, view);
    }
    return ESP_OK;
}

static esp_err_t create_grid_ui(uint32_t refresh_period_ms)
{
    lv_obj_t *screen = lv_screen_active();
    if (!screen) {
        return ESP_ERR_INVALID_STATE;
    }

    const int32_t scr_w = at_least_one(lv_display_get_horizontal_resolution(NULL));
    const int32_t scr_h = at_least_one(lv_display_get_vertical_resolution(NULL));
    const int32_t header_h = scr_h < 240 ? 52 : 68;
    const int32_t passed_h = scr_h < 240 ? 44 : 56;
    const int32_t grid_y = header_h + passed_h + GRID_GAP;
    const int32_t grid_h = at_least_one(scr_h - grid_y);
    const int32_t inner_w = at_least_one(scr_w - 2 * GRID_PADDING);
    const int32_t inner_h = at_least_one(grid_h - 2 * GRID_PADDING);

    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(COLOR_BG_SCREEN), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    if (!create_header(screen, scr_w, header_h)) {
        return ESP_ERR_NO_MEM;
    }

    ESP_RETURN_ON_ERROR(create_passed_card(screen, scr_w, header_h + GRID_GAP, passed_h),
                        TAG, "passed card creation failed");

    lv_obj_t *grid = create_grid_container(screen, scr_w, grid_y, grid_h);
    if (!grid) {
        return ESP_ERR_NO_MEM;
    }

    if (s_monitor.pin_count == 0) {
        lv_obj_t *empty = lv_label_create(grid);
        if (!empty) {
            return ESP_ERR_NO_MEM;
        }
        lv_label_set_text(empty, "Add GPIOs in main.c");
        lv_obj_set_style_text_color(empty, lv_color_hex(COLOR_TEXT_SECONDARY), 0);
        lv_obj_center(empty);
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(create_pin_cards(grid, scr_w, inner_w, inner_h, grid_h), TAG, "pin card creation failed");

    s_monitor.refresh_timer = lv_timer_create(refresh_timer_cb, refresh_period_ms > 0 ? refresh_period_ms : 100, NULL);
    return s_monitor.refresh_timer ? ESP_OK : ESP_ERR_NO_MEM;
}

/* Public API */

esp_err_t gpio_monitor_init(const gpio_monitor_config_t *config)
{
    if (s_monitor.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_RETURN_ON_ERROR(validate_config(config), TAG, "invalid monitor configuration");

    s_monitor.pin_count = config->pin_count;
    s_monitor.refresh_period_ms = config->refresh_period_ms;
    s_monitor.pins = calloc(s_monitor.pin_count, sizeof(*s_monitor.pins));
    if (s_monitor.pin_count > 0 && !s_monitor.pins) {
        return ESP_ERR_NO_MEM;
    }
    if (s_monitor.pin_count > 0) {
        memcpy(s_monitor.pins, config->pins, s_monitor.pin_count * sizeof(*s_monitor.pins));
    }

    s_monitor.output_enabled = config->initial_mode == GPIO_MODE_OUTPUT;
    esp_err_t err = configure_all_gpios();
    if (err != ESP_OK) {
        reset_all_gpios();
        free(s_monitor.pins);
        memset(&s_monitor, 0, sizeof(s_monitor));
        ESP_LOGE(TAG, "GPIO initialization failed: %s", esp_err_to_name(err));
        return err;
    }
    s_monitor.initialized = true;
    return ESP_OK;
}

esp_err_t gpio_monitor_start_ui(void)
{
    if (!s_monitor.initialized || s_monitor.ui_started) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!lv_display_get_default()) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_RETURN_ON_ERROR(create_grid_ui(s_monitor.refresh_period_ms), TAG, "UI initialization failed");
    s_monitor.ui_started = true;
    return ESP_OK;
}

esp_err_t gpio_monitor_set_output_enabled(bool output_enabled)
{
    if (!s_monitor.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    const bool prev = s_monitor.output_enabled;
    s_monitor.output_enabled = output_enabled;

    for (size_t i = 0; i < s_monitor.pin_count; ++i) {
        esp_err_t err = configure_gpio(i);
        if (err != ESP_OK) {
            s_monitor.output_enabled = prev;
            for (size_t j = 0; j < i; ++j) {
                esp_err_t re = configure_gpio(j);
                if (re != ESP_OK) {
                    ESP_LOGE(TAG, "Restore GPIO%d mode failed: %s", s_monitor.pins[j].gpio_num, esp_err_to_name(re));
                }
            }
            return err;
        }
    }

    reset_passed_state();
    if (s_monitor.ui_started) {
        if (s_monitor.mode_label) {
            lv_label_set_text(s_monitor.mode_label, output_enabled ? "OUT" : "IN");
        }
        for (size_t i = 0; i < s_monitor.pin_count; ++i) {
            update_pin_view(i, false);
        }
    }
    return ESP_OK;
}

esp_err_t gpio_monitor_set_level(gpio_num_t gpio_num, uint32_t level)
{
    int index = find_pin_index(gpio_num);
    if (index < 0) {
        return ESP_ERR_NOT_FOUND;
    }
    if (!s_monitor.output_enabled) {
        return ESP_ERR_INVALID_STATE;
    }
    return gpio_set_level(gpio_num, level ? 1 : 0);
}
