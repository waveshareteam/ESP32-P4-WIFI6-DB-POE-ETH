#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_board_manager_includes.h"
#include "esp_lcd_touch_gt911.h"

#include "esp_lcd_st7701.h"

static const char *TAG = "WAVESHARE_DISPLAY_DSI";

// Panel-specific initialization sequence preserved from the reference Waveshare BSP.
static const st7701_lcd_init_cmd_t st7701_init_cmds[] = {
    {0x11, (uint8_t[]){0x00}, 0, 120},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xEF, (uint8_t[]){0x08}, 1, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC0, (uint8_t[]){0x63, 0x00}, 2, 0},
    {0xC1, (uint8_t[]){0x0C, 0x02}, 2, 0},
    {0xC2, (uint8_t[]){0x01, 0x07}, 2, 0},
    {0xCC, (uint8_t[]){0x10}, 1, 0},
    {0xB0, (uint8_t[]){0xCD, 0x18, 0x1F, 0x0F, 0x13, 0x08, 0x09, 0x08, 0x08, 0x24, 0x03, 0x10, 0x0E, 0x21, 0x24, 0x0B}, 16, 0},
    {0xB1, (uint8_t[]){0xC3, 0x0F, 0x18, 0x0B, 0x0F, 0x05, 0x09, 0x09, 0x08, 0x24, 0x06, 0x13, 0x13, 0x28, 0x2D, 0x15}, 16, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
    {0xB0, (uint8_t[]){0x5D}, 1, 0},
    {0xB1, (uint8_t[]){0x3F}, 1, 0},
    {0xB2, (uint8_t[]){0x82}, 1, 0},
    {0xB3, (uint8_t[]){0x80}, 1, 0},
    {0xB5, (uint8_t[]){0x45}, 1, 0},
    {0xB7, (uint8_t[]){0x85}, 1, 0},
    {0xB8, (uint8_t[]){0x21}, 1, 0},
    {0xB9, (uint8_t[]){0x10, 0x1F}, 2, 0},
    {0xBB, (uint8_t[]){0x03}, 1, 0},
    {0xBC, (uint8_t[]){0x3E}, 1, 0},
    {0xC1, (uint8_t[]){0x78}, 1, 0},
    {0xC2, (uint8_t[]){0x78}, 1, 0},
    {0xD0, (uint8_t[]){0x88}, 1, 100},
    {0xE0, (uint8_t[]){0x00, 0x00, 0x02}, 3, 0},
    {0xE1, (uint8_t[]){0x04, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20}, 11, 0},
    {0xE2, (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 12, 0},
    {0xE3, (uint8_t[]){0x00, 0x00, 0x33, 0x00}, 4, 0},
    {0xE4, (uint8_t[]){0x22, 0x00}, 2, 0},
    {0xE5, (uint8_t[]){0x04, 0x34, 0x9A, 0xA0, 0x06, 0x34, 0x9A, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 16, 0},
    {0xE6, (uint8_t[]){0x00, 0x00, 0x33, 0x00}, 4, 0},
    {0xE7, (uint8_t[]){0x22, 0x00}, 2, 0},
    {0xE8, (uint8_t[]){0x05, 0x34, 0x9A, 0xA0, 0x07, 0x34, 0x9A, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 16, 0},
    {0xEB, (uint8_t[]){0x02, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00}, 7, 0},
    {0xEC, (uint8_t[]){0x00, 0x00}, 2, 0},
    {0xED, (uint8_t[]){0xFA, 0x45, 0x0B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB0, 0x54, 0xAF}, 16, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x00}, 5, 200},
    {0x29, (uint8_t[]){0x00}, 0, 0},
};

esp_err_t lcd_dsi_panel_factory_entry_t(
    esp_lcd_dsi_bus_handle_t dsi_handle,
    dev_display_lcd_config_t *lcd_cfg,
    dev_display_lcd_handles_t *lcd_handles)
{
    ESP_LOGI(TAG, "Strong LCD factory override is active (waveshare_lcd)");

    if (dsi_handle == NULL || lcd_cfg == NULL || lcd_handles == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_lcd_dpi_panel_config_t dpi_config = lcd_cfg->sub_cfg.dsi.dpi_config;
    st7701_vendor_config_t vendor_config = {
        .init_cmds = st7701_init_cmds,
        .init_cmds_size = sizeof(st7701_init_cmds) / sizeof(st7701_init_cmds[0]),
        .mipi_config = {
            .dsi_bus = dsi_handle,
            .dpi_config = &dpi_config,
        },
        .flags = {
            .use_mipi_interface = 1,
        },
    };
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = lcd_cfg->sub_cfg.dsi.reset_gpio_num,
        .rgb_ele_order = lcd_cfg->rgb_ele_order,
        .bits_per_pixel = lcd_cfg->bits_per_pixel,
        .data_endian = lcd_cfg->data_endian,
        .flags = {
            .reset_active_high = lcd_cfg->sub_cfg.dsi.reset_active_high,
        },
        .vendor_config = &vendor_config,
    };

    return esp_lcd_new_panel_st7701(
        lcd_handles->io_handle,
        &panel_config,
        &lcd_handles->panel_handle);
}

// Strong override for the GT911 touch configuration used by this amend.
esp_err_t lcd_touch_factory_entry_t(
    const esp_lcd_panel_io_handle_t io,
    const esp_lcd_touch_config_t *config,
    esp_lcd_touch_handle_t *touch_handle)
{
    if (io == NULL || config == NULL || touch_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return esp_lcd_touch_new_i2c_gt911(io, config, touch_handle);
}
