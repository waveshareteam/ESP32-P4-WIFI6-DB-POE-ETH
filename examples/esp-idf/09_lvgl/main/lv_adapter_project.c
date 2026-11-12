#include "esp_board_device.h"
#include "dev_display_lcd.h"
#include "dev_lcd_touch.h"
#include "lcd_brightness.h"
#include "esp_lv_adapter.h"
#include "esp_log.h"
#include "lv_demos.h"

static const char *TAG = "lv_benchmark";

void app_main(void)
{
    dev_display_lcd_config_t *display_config = NULL;
    dev_display_lcd_handles_t *display_handles = NULL;
    dev_lcd_touch_handles_t *touch_handles = NULL;
    void *brightness = NULL;
    lv_display_t *display = NULL;
    lv_indev_t *touch = NULL;
    bool adapter_initialized = false;
    bool display_initialized = false;
    bool touch_initialized = false;
    bool brightness_initialized = false;

    esp_err_t ret = esp_board_device_get_config("display_lcd", (void **)&display_config);
    if (ret != ESP_OK || display_config == NULL) {
        goto cleanup;
    }
    ret = esp_board_device_init("display_lcd");
    if (ret != ESP_OK) {
        goto cleanup;
    }
    display_initialized = true;
    ret = esp_board_device_get_handle("display_lcd", (void **)&display_handles);
    if (ret != ESP_OK || display_handles == NULL) {
        goto cleanup;
    }
    ret = esp_board_device_init("lcd_touch");
    if (ret != ESP_OK) {
        goto cleanup;
    }
    touch_initialized = true;
    ret = esp_board_device_get_handle("lcd_touch", (void **)&touch_handles);
    if (ret != ESP_OK || touch_handles == NULL) {
        goto cleanup;
    }
    ret = esp_board_device_init("lcd_brightness");
    if (ret != ESP_OK) {
        goto cleanup;
    }
    brightness_initialized = true;
    ret = esp_board_device_get_handle("lcd_brightness", &brightness);
    if (ret != ESP_OK || brightness == NULL) {
        goto cleanup;
    }
    esp_lv_adapter_config_t adapter_config = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    ret = esp_lv_adapter_init(&adapter_config);
    if (ret != ESP_OK) {
        goto cleanup;
    }
    adapter_initialized = true;
    esp_lv_adapter_display_config_t adapter_display = ESP_LV_ADAPTER_DISPLAY_MIPI_DEFAULT_CONFIG(
        display_handles->panel_handle, display_handles->io_handle, display_config->lcd_width,
        display_config->lcd_height, ESP_LV_ADAPTER_ROTATE_0);
    adapter_display.tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_DEFAULT_MIPI_DSI;
    display = esp_lv_adapter_register_display(&adapter_display);
    if (display == NULL) {
        ret = ESP_FAIL;
        goto cleanup;
    }
    esp_lv_adapter_touch_config_t adapter_touch = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(display, touch_handles->touch_handle);
    touch = esp_lv_adapter_register_touch(&adapter_touch);
    if (touch == NULL) {
        ret = ESP_FAIL;
        goto cleanup;
    }
    ret = esp_lv_adapter_start();
    if (ret != ESP_OK) {
        goto cleanup;
    }
    ret = esp_custom_lcd_brightness_set(brightness, 100);
    if (ret != ESP_OK) {
        goto cleanup;
    }
    ret = esp_lv_adapter_lock(-1);
    if (ret != ESP_OK) {
        goto cleanup;
    }
    lv_demo_benchmark();
    esp_lv_adapter_unlock();
    ESP_LOGI(TAG, "LVGL benchmark started");
    return;

cleanup:
    ESP_LOGE(TAG, "Failed to initialize display: %s", esp_err_to_name(ret != ESP_OK ? ret : ESP_FAIL));
    if (touch != NULL) {
        esp_lv_adapter_unregister_touch(touch);
    }
    if (display != NULL) {
        esp_lv_adapter_unregister_display(display);
    }
    if (adapter_initialized) {
        esp_lv_adapter_deinit();
    }
    if (brightness_initialized) {
        esp_board_device_deinit("lcd_brightness");
    }
    if (touch_initialized) {
        esp_board_device_deinit("lcd_touch");
    }
    if (display_initialized) {
        esp_board_device_deinit("display_lcd");
    }
}
