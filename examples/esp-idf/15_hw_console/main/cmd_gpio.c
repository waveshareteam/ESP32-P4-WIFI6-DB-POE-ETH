#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_console.h"
#include "esp_gpio_vfs.h"
#include "cmd_gpio.h"
#include "cmd_storage.h"
#include "jobs.h"
#include "report.h"

static const char *TAG = "gpio";

/* Expansion header pins, same order as the BSP's header_gpios[] */
static const int s_pins[] = { 2, 3, 4, 5, 20, 21, 22, 23, 24, 25, 26, 27, 32, 33, 36, 45, 46, 47, 48, 53 };
#define PIN_COUNT (sizeof(s_pins) / sizeof(s_pins[0]))

static const int *gpio_header_pins(size_t *count)
{
    *count = PIN_COUNT;
    return s_pins;
}

static bool gpio_on_header(int pin)
{
    for (size_t i = 0; i < PIN_COUNT; i++) {
        if (s_pins[i] == pin) {
            return true;
        }
    }
    return false;
}

/* Non-NULL (reason) when the pin must not be driven right now. Also used as the
 * esp_gpio_vfs veto callback so `echo 1 > .../value` gets EBUSY. */
static const char *gpio_pin_blocked(int pin)
{
    if (pin == 45 && storage_sd_is_mounted()) {
        return "SD power enable (SD mounted)";
    }
    if (pin == 53 && res_owner(RES_AUDIO)) {
        return "PA enable (audio active)";
    }
    return NULL;
}

static const char *vfs_blocked_cb(int pin, void *ctx)
{
    return gpio_pin_blocked(pin);
}

static int cmd_gpioinfo(int argc, char **argv)
{
    size_t count;
    const int *pins = gpio_header_pins(&count);
    printf("expansion header GPIOs (%u), also visible as /gpio<N>/{value,direction}:\n", (unsigned)count);
    for (size_t i = 0; i < count; i++) {
        const char *b = gpio_pin_blocked(pins[i]);
        printf("  GPIO%-3d %s%s\n", pins[i], b ? "BLOCKED: " : "", b ? b : "");
    }
    printf("GPIO45 = SD power enable, GPIO53 = PA enable, GPIO36 = strapping pin\n");
    return 0;
}

static esp_err_t set_output(int pin, int level)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_INPUT_OUTPUT,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&cfg), TAG, "config");
    return gpio_set_level(pin, level);
}

/* ---- gpioset --blink all: toggle every free pin at 1 Hz until killed ---- */
static bool blink_iter(void *arg)
{
    int *level = arg;
    size_t count;
    const int *pins = gpio_header_pins(&count);
    *level ^= 1;
    for (size_t i = 0; i < count; i++) {
        if (!gpio_pin_blocked(pins[i])) {
            gpio_set_level(pins[i], *level);
        }
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    return true;
}

static void blink_cleanup(void *arg)
{
    free(arg);
}

/* gpioset all=1 | gpioset 20=1 21=0 | gpioset --blink all */
static int cmd_gpioset(int argc, char **argv)
{
    size_t count;
    const int *pins = gpio_header_pins(&count);
    if (argc < 2) {
        printf("usage: gpioset all=<0|1> | gpioset <pin>=<v> ... | gpioset --blink all\n");
        return 1;
    }
    if (strcmp(argv[1], "--blink") == 0) {
        if (job_find("gpioset --blink") > 0) {
            printf("blink already running\n");
            return 1;
        }
        for (size_t i = 0; i < count; i++) {
            if (!gpio_pin_blocked(pins[i])) {
                set_output(pins[i], 0);
            }
        }
        int *level = calloc(1, sizeof(int));
        if (!level) {
            return 1;
        }
        job_desc_t d = {
            .name = "gpioset --blink",
            .iter = blink_iter,
            .cleanup = blink_cleanup,
            .ctx = level,
            .stack_size = 3072,
            .res = RES_MAX,
        };
        if (job_start(&d) < 0) {
            free(level);
            return 1;
        }
        report_set("gpio.header", REPORT_MANUAL, NULL, "1 Hz on all free pins");
        return 0;
    }
    for (int i = 1; i < argc; i++) {
        char *eq = strchr(argv[i], '=');
        if (!eq) {
            printf("bad argument %s (expected <pin>=<0|1>)\n", argv[i]);
            return 1;
        }
        int level = atoi(eq + 1) ? 1 : 0;
        if (strncmp(argv[i], "all=", 4) == 0) {
            for (size_t p = 0; p < count; p++) {
                const char *b = gpio_pin_blocked(pins[p]);
                if (b) {
                    printf("skip GPIO%d: %s\n", pins[p], b);
                } else {
                    set_output(pins[p], level);
                }
            }
        } else {
            int pin = atoi(argv[i]);
            if (!gpio_on_header(pin)) {
                printf("GPIO%d is not on the header\n", pin);
                return 1;
            }
            const char *b = gpio_pin_blocked(pin);
            if (b) {
                printf("refusing GPIO%d: %s\n", pin, b);
                return 1;
            }
            set_output(pin, level);
        }
    }
    printf("ok\n");
    report_set("gpio.header", REPORT_MANUAL, NULL, "measure pins with a multimeter");
    return 0;
}

/* gpioget all | gpioget <pin> ...: configure as input (pull-up) and print levels */
static int cmd_gpioget(int argc, char **argv)
{
    size_t count;
    const int *pins = gpio_header_pins(&count);
    bool all = argc < 2 || strcmp(argv[1], "all") == 0;
    for (size_t i = 0; i < count; i++) {
        int pin = pins[i];
        if (!all) {
            bool wanted = false;
            for (int a = 1; a < argc; a++) {
                wanted |= atoi(argv[a]) == pin;
            }
            if (!wanted) {
                continue;
            }
        }
        const char *b = gpio_pin_blocked(pin);
        if (b) {
            printf("GPIO%-3d skipped (%s)\n", pin, b);
            continue;
        }
        gpio_config_t cfg = {
            .pin_bit_mask = 1ULL << pin,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
        };
        gpio_config(&cfg);
        printf("GPIO%-3d = %d\n", pin, gpio_get_level(pin));
    }
    return 0;
}

void register_gpio_commands(void)
{
    const esp_gpio_vfs_config_t vfs_cfg = {
        .pins = s_pins,
        .pin_count = PIN_COUNT,
        .blocked_cb = vfs_blocked_cb,
    };
    ESP_ERROR_CHECK(esp_gpio_vfs_register(&vfs_cfg));
    const esp_console_cmd_t cmds[] = {
        { .command = "gpioinfo", .help = "List expansion header GPIOs", .func = cmd_gpioinfo },
        { .command = "gpioset",  .help = "Drive header pins. gpioset all=1 | gpioset 20=1 21=0 | gpioset --blink all", .func = cmd_gpioset },
        { .command = "gpioget",  .help = "Read header pins as inputs. gpioget all | gpioget 20 21", .func = cmd_gpioget },
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }
}
