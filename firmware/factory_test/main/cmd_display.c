#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_board_device.h"
#include "esp_check.h"
#include "esp_console.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#include "dev_display_lcd.h"
#include "dev_lcd_touch.h"
#include "lcd_brightness.h"
#include "cmd_display.h"
#include "jobs.h"
#include "report.h"

static const char *TAG = "display";
#define STRIP_LINES 32

static esp_lcd_panel_handle_t s_panel;
static uint16_t s_w;
static uint16_t s_h;
static void *s_brightness;
static esp_lcd_touch_handle_t s_touch;

esp_err_t display_ensure(esp_lcd_panel_handle_t *panel, uint16_t *width, uint16_t *height)
{
    if (!s_panel) {
        ESP_RETURN_ON_ERROR(esp_board_device_init("display_lcd"), TAG, "display_lcd");
        dev_display_lcd_handles_t *h = NULL;
        ESP_RETURN_ON_ERROR(esp_board_device_get_handle("display_lcd", (void **)&h), TAG, "handle");
        dev_display_lcd_config_t *cfg = NULL;
        ESP_RETURN_ON_ERROR(esp_board_device_get_config("display_lcd", (void **)&cfg), TAG, "config");
        s_panel = h->panel_handle;
        s_w = cfg->lcd_width;
        s_h = cfg->lcd_height;
        if (esp_board_device_init("lcd_brightness") == ESP_OK) {
            esp_board_device_get_handle("lcd_brightness", &s_brightness);
        }
        printf("lcd %ux%u ready\n", s_w, s_h);
    }
    if (panel) {
        *panel = s_panel;
    }
    if (width) {
        *width = s_w;
    }
    if (height) {
        *height = s_h;
    }
    return ESP_OK;
}

/* RGB565 */
static uint16_t pattern_pixel(const char *pattern, int x, int y)
{
    static const uint16_t bars[8] = { 0xFFFF, 0xFFE0, 0x07FF, 0x07E0, 0xF81F, 0xF800, 0x001F, 0x0000 };
    if (strcmp(pattern, "colorbar") == 0) {
        return bars[(x * 8) / s_w];
    }
    if (strcmp(pattern, "checker") == 0) {
        return (((x / 32) + (y / 32)) & 1) ? 0xFFFF : 0x0000;
    }
    if (strcmp(pattern, "red") == 0) {
        return 0xF800;
    }
    if (strcmp(pattern, "green") == 0) {
        return 0x07E0;
    }
    if (strcmp(pattern, "blue") == 0) {
        return 0x001F;
    }
    if (strcmp(pattern, "white") == 0) {
        return 0xFFFF;
    }
    return 0x0000; /* black */
}

/* fbtest colorbar|red|green|blue|white|black|checker */
static int cmd_fbtest(int argc, char **argv)
{
    const char *pattern = argc > 1 ? argv[1] : "colorbar";
    if (display_ensure(NULL, NULL, NULL) != ESP_OK) {
        return 1;
    }
    if (!res_lock(RES_LCD, "fbtest")) {
        return 1;
    }
    size_t strip_bytes = (size_t)s_w * STRIP_LINES * 2;
    uint16_t *strip = heap_caps_malloc(strip_bytes, MALLOC_CAP_SPIRAM);
    if (!strip) {
        res_unlock(RES_LCD);
        return 1;
    }
    for (int y0 = 0; y0 < s_h; y0 += STRIP_LINES) {
        int lines = (y0 + STRIP_LINES <= s_h) ? STRIP_LINES : s_h - y0;
        for (int y = 0; y < lines; y++) {
            for (int x = 0; x < s_w; x++) {
                strip[y * s_w + x] = pattern_pixel(pattern, x, y0 + y);
            }
        }
        esp_lcd_panel_draw_bitmap(s_panel, 0, y0, s_w, y0 + lines, strip);
    }
    free(strip);
    res_unlock(RES_LCD);
    char note[48];
    snprintf(note, sizeof(note), "%s shown, check visually", pattern);
    report_set("lcd.pattern", REPORT_MANUAL, NULL, note);
    return 0;
}

static int cmd_backlight(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: backlight <0-100>\n");
        return 1;
    }
    if (display_ensure(NULL, NULL, NULL) != ESP_OK || !s_brightness) {
        printf("brightness device unavailable\n");
        return 1;
    }
    int pct = atoi(argv[1]);
    if (pct < 0 || pct > 100) {
        printf("usage: backlight <0-100>\n");
        return 1;
    }
    esp_err_t err = custom_lcd_brightness_set(s_brightness, pct);
    if (err != ESP_OK) {
        printf("failed: %s\n", esp_err_to_name(err));
        return 1;
    }
    report_set("lcd.backlight", REPORT_MANUAL, NULL, "check brightness change");
    return 0;
}

/* evtest: background job printing touch points until killed */
static bool evtest_iter(void *ctx)
{
    uint16_t x[5];
    uint16_t y[5];
    uint16_t strength[5];
    uint8_t n = 0;
    esp_lcd_touch_read_data(s_touch);
    if (esp_lcd_touch_get_coordinates(s_touch, x, y, strength, &n, 5) && n > 0) {
        for (int i = 0; i < n; i++) {
            printf("touch[%d] x=%u y=%u\n", i, x[i], y[i]);
        }
    }
    vTaskDelay(pdMS_TO_TICKS(30));
    return true;
}

static int cmd_evtest(int argc, char **argv)
{
    if (!s_touch) {
        if (esp_board_device_init("lcd_touch") != ESP_OK) {
            printf("touch init failed (GT911 not found on I2C?)\n");
            report_set("lcd.touch", REPORT_FAIL, NULL, "init failed");
            return 1;
        }
        dev_lcd_touch_handles_t *h = NULL;
        esp_board_device_get_handle("lcd_touch", (void **)&h);
        s_touch = h->touch_handle;
    }
    job_desc_t d = {
        .name = "evtest",
        .iter = evtest_iter,
        .ctx = NULL,
        .stack_size = 4096,
        .res = RES_MAX,
    };
    if (job_start(&d) < 0) {
        return 1;
    }
    printf("touch the panel; stop with: kill <id>\n");
    report_set("lcd.touch", REPORT_MANUAL, NULL, "check coordinates");
    return 0;
}

void register_display_commands(void)
{
    const esp_console_cmd_t cmds[] = {
        { .command = "fbtest",    .help = "Fill the panel. Usage: fbtest colorbar|red|green|blue|white|black|checker", .func = cmd_fbtest },
        { .command = "backlight", .help = "Set backlight. Usage: backlight <0-100>", .func = cmd_backlight },
        { .command = "evtest",    .help = "Print touch events until killed", .func = cmd_evtest },
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }
}
