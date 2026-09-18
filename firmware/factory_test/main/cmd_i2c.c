#include <stdio.h>
#include <string.h>
#include "driver/i2c_master.h"
#include "esp_board_periph.h"
#include "esp_check.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "cmd_i2c.h"
#include "jobs.h"
#include "report.h"

#define I2C_TIMEOUT_MS 50

static const char *TAG = "i2c";
static i2c_master_bus_handle_t s_bus;
static uint32_t s_freq = 400000;

static esp_err_t bus_get(i2c_master_bus_handle_t *out)
{
    if (!s_bus) {
        esp_err_t err = esp_board_periph_ref_handle("i2c_master", (void **)&s_bus);
        if (err != ESP_OK) {
            printf("i2c_master init failed: %s\n", esp_err_to_name(err));
            return err;
        }
    }
    *out = s_bus;
    return ESP_OK;
}

/* Every test transaction uses its own device handle at s_freq; the SCL rate is a
 * per-device property of the IDF i2c_master driver, so touch/codec/backlight
 * handles created by Board Manager keep their own speed. */
static esp_err_t dev_open(uint8_t addr, i2c_master_dev_handle_t *dev)
{
    i2c_master_bus_handle_t bus;
    ESP_RETURN_ON_ERROR(bus_get(&bus), TAG, "bus");
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = s_freq,
    };
    return i2c_master_bus_add_device(bus, &cfg, dev);
}

/* i2cconfig --freq <hz> */
static struct {
    struct arg_int *freq;
    struct arg_end *end;
} cfg_args;

static int cmd_i2cconfig(int argc, char **argv)
{
    args_lock();
    int nerrors = arg_parse(argc, argv, (void **)&cfg_args);
    int freq = cfg_args.freq->count ? cfg_args.freq->ival[0] : 0;
    if (nerrors) {
        arg_print_errors(stderr, cfg_args.end, argv[0]);
    }
    args_unlock();
    if (nerrors) {
        return 1;
    }
    if (freq != 100000 && freq != 400000) {
        printf("freq must be 100000 or 400000\n");
        return 1;
    }
    s_freq = (uint32_t)freq;
    printf("i2c SCL = %u Hz (SDA=GPIO7 SCL=GPIO8)\n", (unsigned)s_freq);
    return 0;
}

/* i2cdetect: a real 1-byte read per address at s_freq. i2c_master_probe() is not
 * used because the IDF driver hard-codes it to 100 kHz. */
static int cmd_i2cdetect(int argc, char **argv)
{
    if (!res_lock(RES_I2C, "i2cdetect")) {
        return 1;
    }
    bool found[128] = {0};
    bool quiet = job_is_background();
    if (!quiet) {
        printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
    }
    for (int addr = 0; addr < 128; addr++) {
        if (addr % 16 == 0 && !quiet) {
            printf("%02x:", addr);
        }
        bool scannable = addr >= 0x08 && addr <= 0x77;
        if (scannable) {
            i2c_master_dev_handle_t dev;
            if (dev_open((uint8_t)addr, &dev) == ESP_OK) {
                uint8_t byte;
                found[addr] = i2c_master_receive(dev, &byte, 1, I2C_TIMEOUT_MS) == ESP_OK;
                i2c_master_bus_rm_device(dev);
            }
        }
        if (!quiet) {
            if (found[addr]) {
                printf(" %02x", addr);
            } else {
                printf(scannable ? " --" : "   ");
            }
            if (addr % 16 == 15) {
                printf("\n");
            }
        }
    }
    res_unlock(RES_I2C);

    bool codec = found[0x18];
    bool backlight = found[0x45];
    bool touch = found[0x5d] || found[0x14];
    bool pass = codec && backlight;
    char note[48];
    snprintf(note, sizeof(note), "0x18:%c 0x45:%c gt911:%c",
             codec ? 'y' : 'n', backlight ? 'y' : 'n', touch ? 'y' : 'n');
    char item[32];
    snprintf(item, sizeof(item), "i2c.detect.%uk", (unsigned)(s_freq / 1000));
    report_set(item, pass ? REPORT_PASS : REPORT_FAIL, NULL, note);
    job_report(pass, 0);
    if (!quiet) {
        printf("%s: %s (%s)\n", item, pass ? "PASS" : "FAIL", note);
    }
    return pass ? 0 : 1;
}

/* i2cget -c <addr> -r <reg> [-l <len>] */
static struct {
    struct arg_int *chip;
    struct arg_int *reg;
    struct arg_int *len;
    struct arg_end *end;
} get_args;

static int cmd_i2cget(int argc, char **argv)
{
    args_lock();
    int nerrors = arg_parse(argc, argv, (void **)&get_args);
    int chip = get_args.chip->count ? get_args.chip->ival[0] : -1;
    int reg = get_args.reg->count ? get_args.reg->ival[0] : -1;
    int len = get_args.len->count ? get_args.len->ival[0] : 1;
    if (nerrors) {
        arg_print_errors(stderr, get_args.end, argv[0]);
    }
    args_unlock();
    if (nerrors || chip < 0 || reg < 0 || len < 1 || len > 32) {
        return 1;
    }
    if (!res_lock(RES_I2C, "i2cget")) {
        return 1;
    }
    i2c_master_dev_handle_t dev;
    uint8_t buf[32];
    uint8_t r = (uint8_t)reg;
    esp_err_t err = dev_open((uint8_t)chip, &dev);
    if (err == ESP_OK) {
        err = i2c_master_transmit_receive(dev, &r, 1, buf, len, I2C_TIMEOUT_MS);
        i2c_master_bus_rm_device(dev);
    }
    res_unlock(RES_I2C);
    job_report(err == ESP_OK, err == ESP_OK ? (uint64_t)len : 0);
    if (err != ESP_OK) {
        if (!job_is_background()) {
            printf("read failed: %s\n", esp_err_to_name(err));
        }
        return 1;
    }
    if (!job_is_background()) {
        for (int i = 0; i < len; i++) {
            printf("0x%02x ", buf[i]);
        }
        printf("\n");
    }
    return 0;
}

/* i2cset -c <addr> -r <reg> <data...> */
static struct {
    struct arg_int *chip;
    struct arg_int *reg;
    struct arg_int *data;
    struct arg_end *end;
} set_args;

static int cmd_i2cset(int argc, char **argv)
{
    uint8_t buf[33];
    int n = 0;
    int chip = -1;
    args_lock();
    int nerrors = arg_parse(argc, argv, (void **)&set_args);
    if (!nerrors) {
        chip = set_args.chip->ival[0];
        buf[n++] = (uint8_t)set_args.reg->ival[0];
        for (int i = 0; i < set_args.data->count && n < (int)sizeof(buf); i++) {
            buf[n++] = (uint8_t)set_args.data->ival[i];
        }
    } else {
        arg_print_errors(stderr, set_args.end, argv[0]);
    }
    args_unlock();
    if (nerrors) {
        return 1;
    }
    if (!res_lock(RES_I2C, "i2cset")) {
        return 1;
    }
    i2c_master_dev_handle_t dev;
    esp_err_t err = dev_open((uint8_t)chip, &dev);
    if (err == ESP_OK) {
        err = i2c_master_transmit(dev, buf, n, I2C_TIMEOUT_MS);
        i2c_master_bus_rm_device(dev);
    }
    res_unlock(RES_I2C);
    job_report(err == ESP_OK, 0);
    if (err != ESP_OK && !job_is_background()) {
        printf("write failed: %s\n", esp_err_to_name(err));
    }
    return err == ESP_OK ? 0 : 1;
}

/* i2cdump -c <addr> */
static struct {
    struct arg_int *chip;
    struct arg_end *end;
} dump_args;

static int cmd_i2cdump(int argc, char **argv)
{
    args_lock();
    int nerrors = arg_parse(argc, argv, (void **)&dump_args);
    int chip = nerrors ? -1 : dump_args.chip->ival[0];
    if (nerrors) {
        arg_print_errors(stderr, dump_args.end, argv[0]);
    }
    args_unlock();
    if (nerrors) {
        return 1;
    }
    if (!res_lock(RES_I2C, "i2cdump")) {
        return 1;
    }
    i2c_master_dev_handle_t dev;
    esp_err_t err = dev_open((uint8_t)chip, &dev);
    if (err == ESP_OK) {
        printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
        for (int reg = 0; reg < 256; reg++) {
            uint8_t r = (uint8_t)reg;
            uint8_t v = 0;
            if (reg % 16 == 0) {
                printf("%02x:", reg);
            }
            if (i2c_master_transmit_receive(dev, &r, 1, &v, 1, I2C_TIMEOUT_MS) == ESP_OK) {
                printf(" %02x", v);
            } else {
                printf(" XX");
            }
            if (reg % 16 == 15) {
                printf("\n");
            }
        }
        i2c_master_bus_rm_device(dev);
    } else {
        printf("open failed: %s\n", esp_err_to_name(err));
    }
    res_unlock(RES_I2C);
    return err == ESP_OK ? 0 : 1;
}

void register_i2c_commands(void)
{
    cfg_args.freq = arg_int1(NULL, "freq", "<hz>", "100000 or 400000");
    cfg_args.end = arg_end(1);
    get_args.chip = arg_int1("c", "chip", "<addr>", "7-bit address");
    get_args.reg = arg_int1("r", "register", "<reg>", "register");
    get_args.len = arg_int0("l", "length", "<n>", "bytes to read (1..32)");
    get_args.end = arg_end(2);
    set_args.chip = arg_int1("c", "chip", "<addr>", "7-bit address");
    set_args.reg = arg_int1("r", "register", "<reg>", "register");
    set_args.data = arg_intn(NULL, NULL, "<data>", 0, 32, "bytes to write");
    set_args.end = arg_end(2);
    dump_args.chip = arg_int1("c", "chip", "<addr>", "7-bit address");
    dump_args.end = arg_end(1);

    const esp_console_cmd_t cmds[] = {
        { .command = "i2cconfig", .help = "Set SCL frequency for the i2c* commands. Usage: i2cconfig --freq 400000", .func = cmd_i2cconfig, .argtable = &cfg_args },
        { .command = "i2cdetect", .help = "Scan the shared bus (GPIO7/8) at the configured frequency", .func = cmd_i2cdetect },
        { .command = "i2cget",    .help = "Read register(s). Usage: i2cget -c 0x18 -r 0x00 [-l 4]", .func = cmd_i2cget, .argtable = &get_args },
        { .command = "i2cset",    .help = "Write register. Usage: i2cset -c 0x18 -r 0x00 0x12", .func = cmd_i2cset, .argtable = &set_args },
        { .command = "i2cdump",   .help = "Dump registers 0x00-0xff. Usage: i2cdump -c 0x18", .func = cmd_i2cdump, .argtable = &dump_args },
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }
}
