#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_check.h"
#include "esp_console.h"
#include "esp_vfs_fat.h"
#include "usb/usb_host.h"
#include "usb/msc_host.h"
#include "usb/msc_host_vfs.h"
#include "cmd_usb.h"
#include "jobs.h"
#include "report.h"

static const char *TAG = "usb";
#define BIT_DEV_CONNECTED    (1 << 0)
#define BIT_DEV_DISCONNECTED (1 << 1)

static EventGroupHandle_t s_events;
static TaskHandle_t s_host_task;
static bool s_host_installed;
static uint8_t s_dev_addr;
static msc_host_device_handle_t s_msc_dev;
static msc_host_vfs_handle_t s_vfs;
static char s_mount_path[16];

static void usb_host_task(void *arg)
{
    while (true) {
        uint32_t flags;
        usb_host_lib_handle_events(portMAX_DELAY, &flags);
        if (flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            usb_host_device_free_all();
        }
    }
}

static void msc_event_cb(const msc_host_event_t *event, void *arg)
{
    if (event->event == MSC_DEVICE_CONNECTED) {
        s_dev_addr = event->device.address;
        xEventGroupSetBits(s_events, BIT_DEV_CONNECTED);
        printf("usb: mass storage device connected (address %u)\n", s_dev_addr);
    } else if (event->event == MSC_DEVICE_DISCONNECTED) {
        xEventGroupClearBits(s_events, BIT_DEV_CONNECTED);
        xEventGroupSetBits(s_events, BIT_DEV_DISCONNECTED);
        printf("usb: mass storage device disconnected\n");
    }
}

static esp_err_t host_install(void)
{
    if (s_host_installed) {
        return ESP_OK;
    }
    if (!s_events) {
        s_events = xEventGroupCreate();
    }
    const usb_host_config_t host_config = { .intr_flags = ESP_INTR_FLAG_LOWMED };
    ESP_RETURN_ON_ERROR(usb_host_install(&host_config), TAG, "usb_host_install");
    xTaskCreate(usb_host_task, "usb_events", 4096, NULL, 2, &s_host_task);
    const msc_host_driver_config_t msc_config = {
        .create_backround_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .callback = msc_event_cb,
    };
    esp_err_t err = msc_host_install(&msc_config);
    if (err != ESP_OK) {
        vTaskDelete(s_host_task);
        usb_host_uninstall();
        printf("msc_host_install failed: %s\n", esp_err_to_name(err));
        return err;
    }
    s_host_installed = true;
    printf("usb host started, waiting for devices...\n");
    return ESP_OK;
}

bool usb_msc_is_mounted(void)
{
    return s_vfs != NULL;
}

esp_err_t usb_msc_mount(const char *path, uint32_t timeout_ms)
{
    if (s_vfs) {
        printf("%s already mounted\n", s_mount_path);
        return ESP_ERR_INVALID_STATE;
    }
    ESP_RETURN_ON_ERROR(host_install(), TAG, "host");
    if (!res_lock(RES_USB, "mount")) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = ESP_OK;
    if (!(xEventGroupGetBits(s_events) & BIT_DEV_CONNECTED)) {
        printf("waiting %u ms for a USB mass storage device...\n", (unsigned)timeout_ms);
        EventBits_t bits = xEventGroupWaitBits(s_events, BIT_DEV_CONNECTED, pdFALSE, pdFALSE,
                                               pdMS_TO_TICKS(timeout_ms));
        if (!(bits & BIT_DEV_CONNECTED)) {
            printf("no device\n");
            report_set("usb.msc.mount", REPORT_SKIP, NULL, "no device");
            err = ESP_ERR_TIMEOUT;
            goto out;
        }
    }
    xEventGroupClearBits(s_events, BIT_DEV_DISCONNECTED);
    err = msc_host_install_device(s_dev_addr, &s_msc_dev);
    if (err != ESP_OK) {
        printf("msc_host_install_device: %s\n", esp_err_to_name(err));
        report_set("usb.msc.mount", REPORT_FAIL, NULL, esp_err_to_name(err));
        goto out;
    }
    msc_host_device_info_t info;
    msc_host_get_device_info(s_msc_dev, &info);
    printf("VID:PID %04x:%04x  %llu MB (%u sectors x %u bytes)\n", info.idVendor, info.idProduct,
           (unsigned long long)info.sector_count * info.sector_size / (1024 * 1024),
           (unsigned)info.sector_count, (unsigned)info.sector_size);
    const esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 4,
        .allocation_unit_size = 16 * 1024,
    };
    err = msc_host_vfs_register(s_msc_dev, path, &mount_config, &s_vfs);
    if (err != ESP_OK) {
        printf("mount failed: %s\n", esp_err_to_name(err));
        msc_host_uninstall_device(s_msc_dev);
        s_msc_dev = NULL;
        report_set("usb.msc.mount", REPORT_FAIL, NULL, esp_err_to_name(err));
        goto out;
    }
    strlcpy(s_mount_path, path, sizeof(s_mount_path));
    char value[24];
    snprintf(value, sizeof(value), "%04x:%04x", info.idVendor, info.idProduct);
    report_set("usb.msc.mount", REPORT_PASS, value, NULL);
    printf("mounted at %s\n", path);
out:
    res_unlock(RES_USB);
    return err;
}

esp_err_t usb_msc_umount(void)
{
    if (!s_vfs) {
        return ESP_OK;
    }
    if (!res_lock(RES_USB, "umount")) {
        return ESP_ERR_INVALID_STATE;
    }
    msc_host_vfs_unregister(s_vfs);
    s_vfs = NULL;
    msc_host_uninstall_device(s_msc_dev);
    s_msc_dev = NULL;
    res_unlock(RES_USB);
    printf("unmounted %s\n", s_mount_path);
    return ESP_OK;
}

static int cmd_lsusb(int argc, char **argv)
{
    if (host_install() != ESP_OK) {
        return 1;
    }
    uint8_t addrs[8];
    int num = 0;
    usb_host_device_addr_list_fill(sizeof(addrs), addrs, &num);
    printf("%d device(s) enumerated\n", num);
    if (s_msc_dev) {
        msc_host_print_descriptors(s_msc_dev);
    } else if (xEventGroupGetBits(s_events) & BIT_DEV_CONNECTED) {
        printf("MSC device at address %u (not mounted; use: mount -t usb /usb)\n", s_dev_addr);
    }
    return 0;
}

void register_usb_commands(void)
{
    const esp_console_cmd_t cmd = {
        .command = "lsusb",
        .help = "Start USB host and list enumerated devices",
        .func = cmd_lsusb,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
