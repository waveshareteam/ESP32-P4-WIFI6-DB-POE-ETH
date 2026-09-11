#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "lv_demos.h"

static const char *TAG = "lv_benchmark";

void app_main(void)
{
    const bsp_display_cfg_t display_config = {
        .lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation = ESP_LV_ADAPTER_ROTATE_0,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_DEFAULT_MIPI_DSI,
    };

    ESP_LOGI(TAG, "Initializing BSP display (%dx%d)", BSP_LCD_H_RES, BSP_LCD_V_RES);
    lv_display_t *display = bsp_display_start_with_config(&display_config);
    if (display == NULL) {
        ESP_LOGE(TAG, "Failed to initialize BSP display");
        return;
    }

    esp_err_t ret = bsp_display_backlight_on();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable backlight: %s", esp_err_to_name(ret));
        bsp_display_stop(display);
        return;
    }

    if (!bsp_display_lock(-1)) {
        ESP_LOGE(TAG, "Failed to lock LVGL");
        bsp_display_stop(display);
        return;
    }
    lv_demo_benchmark();
    bsp_display_unlock();

    ESP_LOGI(TAG, "LVGL benchmark started");
}
