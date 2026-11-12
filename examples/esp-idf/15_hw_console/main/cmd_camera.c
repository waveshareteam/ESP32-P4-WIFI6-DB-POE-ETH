#include <stdio.h>
#include <string.h>
#include "esp_board_device.h"
#include "esp_console.h"
#include "dev_display_lcd.h"
#include "camera_preview.h"
#include "cmd_camera.h"
#include "cmd_display.h"
#include "jobs.h"
#include "report.h"

static bool preview_iter(void *ctx)
{
    esp_lcd_panel_handle_t panel;
    dev_display_lcd_config_t *cfg = NULL;
    if (display_ensure(&panel, NULL, NULL) != ESP_OK ||
        esp_board_device_get_config("display_lcd", (void **)&cfg) != ESP_OK) {
        return false;
    }
    esp_err_t err = camera_preview_run(panel, cfg);   /* blocks until job_should_stop() */
    if (err != ESP_OK) {
        printf("camera preview error: %s\n", esp_err_to_name(err));
    }
    return false;
}

static void preview_cleanup(void *ctx)
{
    esp_board_device_deinit("camera");
}

/* v4l2-ctl --stream | v4l2-ctl --stop */
static int cmd_v4l2(int argc, char **argv)
{
    if (argc >= 2 && strcmp(argv[1], "--stream") == 0) {
        if (job_find("v4l2-ctl") > 0) {
            printf("preview already running\n");
            return 1;
        }
        if (display_ensure(NULL, NULL, NULL) != ESP_OK) {
            return 1;
        }
        esp_err_t err = esp_board_device_init("camera");
        if (err != ESP_OK) {
            printf("camera init failed: %s (sensor connected? OV5647/SC2336 selected in menuconfig?)\n",
                   esp_err_to_name(err));
            report_set("camera.preview", REPORT_FAIL, NULL, esp_err_to_name(err));
            return 1;
        }
        job_desc_t d = {
            .name = "v4l2-ctl",
            .iter = preview_iter,
            .cleanup = preview_cleanup,
            .stack_size = 16384,
            .res = RES_CAMERA,
        };
        if (job_start(&d) < 0) {
            esp_board_device_deinit("camera");
            return 1;
        }
        report_set("camera.preview", REPORT_MANUAL, NULL, "check live image on LCD");
        return 0;
    }
    if (argc >= 2 && strcmp(argv[1], "--stop") == 0) {
        int id = job_find("v4l2-ctl");
        if (id < 0) {
            printf("preview not running\n");
            return 1;
        }
        return job_kill(id) == ESP_OK ? 0 : 1;
    }
    printf("usage: v4l2-ctl --stream | v4l2-ctl --stop\n");
    return 1;
}

void register_camera_commands(void)
{
    const esp_console_cmd_t cmd = {
        .command = "v4l2-ctl",
        .help = "Camera preview on the LCD. Usage: v4l2-ctl --stream | v4l2-ctl --stop",
        .func = cmd_v4l2,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
