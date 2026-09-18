#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "dev_display_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Stream the Board Manager "camera" device to the panel. Blocks until
 * job_should_stop() becomes true, so it must run inside a job task. */
esp_err_t camera_preview_run(esp_lcd_panel_handle_t panel, const dev_display_lcd_config_t *lcd_cfg);

#ifdef __cplusplus
}
#endif
