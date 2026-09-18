#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_psram.h"
#include "esp_heap_caps.h"
#include "esp_app_desc.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_console.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cmd_sys.h"
#include "report.h"

static int cmd_uname(int argc, char **argv)
{
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);
    const esp_app_desc_t *app = esp_app_get_description();

    printf("chip:      ESP32-P4 rev v%d.%d, %d core(s)\n",
           chip.revision / 100, chip.revision % 100, chip.cores);
    printf("idf:       %s\n", app->idf_ver);
    printf("app:       %s %s (%s %s)\n", app->project_name, app->version, app->date, app->time);
    printf("flash:     %" PRIu32 " MB\n", flash_size / (1024 * 1024));
    printf("psram:     %u MB\n", (unsigned)(esp_psram_get_size() / (1024 * 1024)));
    printf("base mac:  %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    report_set("sys.uname", REPORT_PASS, NULL, NULL);
    return 0;
}

static int cmd_free(int argc, char **argv)
{
    printf("%-10s %12s %12s %12s\n", "", "total", "free", "largest");
    printf("%-10s %12u %12u %12u\n", "internal",
           (unsigned)heap_caps_get_total_size(MALLOC_CAP_INTERNAL),
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
           (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    printf("%-10s %12u %12u %12u\n", "psram",
           (unsigned)heap_caps_get_total_size(MALLOC_CAP_SPIRAM),
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
           (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    return 0;
}

static int cmd_reboot(int argc, char **argv)
{
    printf("rebooting...\n");
    fflush(stdout);
    esp_restart();
    return 0;
}

static int cmd_nvs(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "test") != 0) {
        printf("usage: nvs test\n");
        return 1;
    }
    nvs_handle_t h;
    esp_err_t err = nvs_open("factory", NVS_READWRITE, &h);
    if (err != ESP_OK) {
        printf("nvs_open failed: %s\n", esp_err_to_name(err));
        report_set("nvs.rw", REPORT_FAIL, NULL, esp_err_to_name(err));
        return 1;
    }
    uint32_t count = 0;
    err = nvs_get_u32(h, "boot_count", &count);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        printf("nvs_get_u32 failed: %s\n", esp_err_to_name(err));
        nvs_close(h);
        report_set("nvs.rw", REPORT_FAIL, NULL, esp_err_to_name(err));
        return 1;
    }
    count++;
    err = nvs_set_u32(h, "boot_count", count);
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);
    if (err != ESP_OK) {
        printf("nvs write failed: %s\n", esp_err_to_name(err));
        report_set("nvs.rw", REPORT_FAIL, NULL, esp_err_to_name(err));
        return 1;
    }
    char value[16];
    snprintf(value, sizeof(value), "count=%" PRIu32, count);
    printf("nvs ok, boot_count=%" PRIu32 "\n", count);
    report_set("nvs.rw", REPORT_PASS, value, NULL);
    return 0;
}

void register_sys_commands(void)
{
    const esp_console_cmd_t cmds[] = {
        { .command = "uname",  .help = "Show chip / IDF / flash / PSRAM info. Usage: uname -a", .func = cmd_uname },
        { .command = "free",   .help = "Show heap usage (internal / PSRAM)",                    .func = cmd_free },
        { .command = "reboot", .help = "Restart the chip",                                      .func = cmd_reboot },
        { .command = "nvs",    .help = "NVS read/write test. Usage: nvs test",                  .func = cmd_nvs },
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }
}
