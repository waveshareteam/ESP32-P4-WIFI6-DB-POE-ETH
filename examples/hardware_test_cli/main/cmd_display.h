#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

/* fbtest / backlight / evtest */
void register_display_commands(void);

/* Initialise the MIPI-DSI panel (and backlight) through Board Manager if not done
 * yet. Any output pointer may be NULL. Used by cmd_camera for the preview. */
esp_err_t display_ensure(esp_lcd_panel_handle_t *panel, uint16_t *width, uint16_t *height);

#ifdef __cplusplus
}
#endif
