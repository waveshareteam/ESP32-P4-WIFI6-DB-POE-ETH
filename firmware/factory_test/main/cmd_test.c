#include <stdio.h>
#include <string.h>
#include "esp_console.h"
#include "cmd_storage.h"
#include "cmd_test.h"
#include "cmd_usb.h"
#include "report.h"

static void run(const char *cmdline)
{
    int ret = 0;
    printf("\n>>> %s\n", cmdline);
    esp_err_t err = esp_console_run(cmdline, &ret);
    if (err != ESP_OK || ret != 0) {
        printf("<<< %s: failed\n", cmdline);
    }
}

/* Runs every item that can be judged automatically. Wi-Fi needs credentials and
 * the display/camera/audio items need a human, so they are left to the tester. */
static void test_all(void)
{
    report_clear();
    run("uname -a");
    run("nvs test");
    run("i2cconfig --freq 100000");
    run("i2cdetect");
    run("i2cconfig --freq 400000");
    run("i2cdetect");

    if (storage_sd_is_mounted()) {
        run("umount /sdcard");
    }
    run("mount -t sd -o freq=20 /sdcard");
    if (storage_sd_is_mounted()) {
        run("dd if=/dev/urandom of=/sdcard/t.bin bs=64k count=256");
        run("dd if=/sdcard/t.bin of=/dev/null bs=64k");
        run("umount /sdcard");
        run("mount -t sd -o freq=40 /sdcard");
        run("dd if=/dev/urandom of=/sdcard/t.bin bs=64k count=256");
        run("dd if=/sdcard/t.bin of=/dev/null bs=64k");
        run("umount /sdcard");
    } else {
        report_set("sd.mount.20M", REPORT_SKIP, NULL, "no card");
    }

    run("ifup eth0");

    if (!usb_msc_is_mounted()) {
        run("mount -t usb /usb");
    }
    if (usb_msc_is_mounted()) {
        run("dd if=/dev/urandom of=/usb/t.bin bs=64k count=256");
        run("dd if=/usb/t.bin of=/dev/null bs=64k");
        run("umount /usb");
    }

    printf("\n");
    report_print();
}

static int cmd_test(int argc, char **argv)
{
    if (argc >= 2 && strcmp(argv[1], "report") == 0) {
        report_print();
        return 0;
    }
    if (argc >= 2 && strcmp(argv[1], "clear") == 0) {
        report_clear();
        return 0;
    }
    if (argc >= 2 && strcmp(argv[1], "all") == 0) {
        test_all();
        return 0;
    }
    printf("usage: test all | test report | test clear\n");
    return 1;
}

void register_test_commands(void)
{
    const esp_console_cmd_t cmd = {
        .command = "test",
        .help = "test all (automatic items) | test report | test clear",
        .func = cmd_test,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
