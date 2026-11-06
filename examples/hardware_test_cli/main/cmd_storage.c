#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include "esp_board_device.h"
#include "esp_check.h"
#include "esp_console.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "dev_fs_fat.h"
#include "cmd_storage.h"
#include "cmd_usb.h"
#include "jobs.h"
#include "report.h"

static const char *TAG = "storage";
static int s_sd_freq_mhz;   /* 0 = not mounted */

bool storage_sd_is_mounted(void)
{
    return s_sd_freq_mhz != 0;
}

int storage_sd_freq_mhz(void)
{
    return s_sd_freq_mhz;
}

/* Board Manager's fs_sdcard device is generated with SDMMC_FREQ_HIGHSPEED. The
 * clock is switched by overriding the generated config before init and restoring
 * it after deinit, so the 20 MHz / 40 MHz choice never leaves this file. */
static esp_err_t sd_mount(int freq_mhz)
{
    if (s_sd_freq_mhz) {
        printf("/sdcard already mounted at %d MHz, umount first\n", s_sd_freq_mhz);
        return ESP_ERR_INVALID_STATE;
    }
    dev_fs_fat_config_t *gen = NULL;
    ESP_RETURN_ON_ERROR(esp_board_device_get_config("fs_sdcard", (void **)&gen), TAG, "get config");
    dev_fs_fat_config_t cfg = *gen;
    cfg.frequency = (freq_mhz == 20) ? SDMMC_FREQ_DEFAULT : SDMMC_FREQ_HIGHSPEED;
    ESP_RETURN_ON_ERROR(esp_board_device_override_config("fs_sdcard", &cfg, sizeof(cfg)), TAG, "override");
    esp_err_t err = esp_board_device_init("fs_sdcard");
    if (err != ESP_OK) {
        esp_board_device_restore_config("fs_sdcard");
        printf("mount failed: %s\n", esp_err_to_name(err));
        char item[32];
        snprintf(item, sizeof(item), "sd.mount.%dM", freq_mhz);
        report_set(item, REPORT_FAIL, NULL, esp_err_to_name(err));
        return err;
    }
    dev_fs_fat_handle_t *h = NULL;
    esp_board_device_get_handle("fs_sdcard", (void **)&h);
    sdmmc_card_print_info(stdout, h->card);
    printf("requested %d MHz, negotiated %d kHz\n", freq_mhz, h->card->real_freq_khz);
    s_sd_freq_mhz = freq_mhz;
    char item[32], value[24];
    snprintf(item, sizeof(item), "sd.mount.%dM", freq_mhz);
    snprintf(value, sizeof(value), "%d kHz", h->card->real_freq_khz);
    report_set(item, REPORT_PASS, value, h->card->cid.name);
    return ESP_OK;
}

static esp_err_t sd_umount(void)
{
    if (!s_sd_freq_mhz) {
        return ESP_OK;
    }
    if (!res_lock(RES_SD, "umount")) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = esp_board_device_deinit("fs_sdcard");
    esp_board_device_restore_config("fs_sdcard");
    s_sd_freq_mhz = 0;
    res_unlock(RES_SD);
    return err;
}

/* mount -t sd [-o freq=20|40] /sdcard    |    mount -t usb /usb */
static int cmd_mount(int argc, char **argv)
{
    const char *type = NULL;
    const char *opts = "";
    const char *path = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            type = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            opts = argv[++i];
        } else {
            path = argv[i];
        }
    }
    if (!type) {
        printf("usage: mount -t sd [-o freq=20|40] /sdcard\n       mount -t usb /usb\n");
        return 1;
    }
    if (strcmp(type, "sd") == 0) {
        int freq = 40;
        const char *f = strstr(opts, "freq=");
        if (f) {
            freq = atoi(f + 5);
        }
        if (freq != 20 && freq != 40) {
            printf("freq must be 20 or 40\n");
            return 1;
        }
        return sd_mount(freq) == ESP_OK ? 0 : 1;
    }
    if (strcmp(type, "usb") == 0) {
        return usb_msc_mount(path ? path : "/usb", 10000) == ESP_OK ? 0 : 1;
    }
    printf("unknown type %s\n", type);
    return 1;
}

static int cmd_umount(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: umount /sdcard | umount /usb\n");
        return 1;
    }
    if (strcmp(argv[1], "/sdcard") == 0) {
        return sd_umount() == ESP_OK ? 0 : 1;
    }
    if (strcmp(argv[1], "/usb") == 0) {
        return usb_msc_umount() == ESP_OK ? 0 : 1;
    }
    printf("unknown mount point %s\n", argv[1]);
    return 1;
}

static void df_one(const char *path)
{
    uint64_t total = 0;
    uint64_t free_bytes = 0;
    if (esp_vfs_fat_info(path, &total, &free_bytes) == ESP_OK) {
        printf("%-10s %10llu MB total %10llu MB free\n", path,
               (unsigned long long)(total >> 20), (unsigned long long)(free_bytes >> 20));
    }
}

static int cmd_df(int argc, char **argv)
{
    if (storage_sd_is_mounted()) {
        df_one("/sdcard");
    }
    if (usb_msc_is_mounted()) {
        df_one("/usb");
    }
    if (!storage_sd_is_mounted() && !usb_msc_is_mounted()) {
        printf("nothing mounted\n");
    }
    return 0;
}

static int cmd_ls(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "/";
    DIR *d = opendir(path);
    if (!d) {
        printf("cannot open %s\n", path);
        return 1;
    }
    struct dirent *e;
    char full[300];
    struct stat st;
    while ((e = readdir(d)) != NULL) {
        bool has_slash = path[strlen(path) - 1] == '/';
        snprintf(full, sizeof(full), "%s%s%s", path, has_slash ? "" : "/", e->d_name);
        if (stat(full, &st) == 0) {
            printf("%c %10ld  %s\n", S_ISDIR(st.st_mode) ? 'd' : '-', (long)st.st_size, e->d_name);
        } else {
            printf("? %10s  %s\n", "", e->d_name);
        }
    }
    closedir(d);
    return 0;
}

/* ---- dd ---- */

#define DD_SEED 0x12345678u

static uint32_t prng_next(uint32_t *s)
{
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *s = x;
}

static void prng_fill(uint8_t *buf, size_t n, uint32_t *seed)
{
    uint32_t *w = (uint32_t *)buf;
    for (size_t i = 0; i < n / 4; i++) {
        w[i] = prng_next(seed);
    }
}

static size_t prng_verify(const uint8_t *buf, size_t n, uint32_t *seed)
{
    const uint32_t *w = (const uint32_t *)buf;
    size_t bad = 0;
    for (size_t i = 0; i < n / 4; i++) {
        if (w[i] != prng_next(seed)) {
            bad++;
        }
    }
    return bad;
}

static size_t parse_size(const char *s)
{
    char *end;
    size_t v = strtoul(s, &end, 10);
    if (*end == 'k' || *end == 'K') {
        v <<= 10;
    } else if (*end == 'm' || *end == 'M') {
        v <<= 20;
    }
    return v;
}

static res_id_t res_for_path(const char *p)
{
    if (strncmp(p, "/sdcard", 7) == 0) {
        return RES_SD;
    }
    if (strncmp(p, "/usb", 4) == 0) {
        return RES_USB;
    }
    return RES_MAX;
}

/* dd if=<src> of=<dst> [bs=64k] [count=N]
 *   src: /dev/urandom (seeded PRNG), /dev/zero, or a file
 *   dst: /dev/null (verify against the PRNG sequence), or a file */
static int cmd_dd(int argc, char **argv)
{
    const char *in = NULL;
    const char *out = NULL;
    size_t bs = 64 * 1024;
    size_t count = 0;
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "if=", 3) == 0) {
            in = argv[i] + 3;
        } else if (strncmp(argv[i], "of=", 3) == 0) {
            out = argv[i] + 3;
        } else if (strncmp(argv[i], "bs=", 3) == 0) {
            bs = parse_size(argv[i] + 3);
        } else if (strncmp(argv[i], "count=", 6) == 0) {
            count = strtoul(argv[i] + 6, NULL, 10);
        }
    }
    if (!in || !out || bs < 512 || bs > (4u << 20) || (bs % 4) != 0) {
        printf("usage: dd if=<src> of=<dst> [bs=64k] [count=N]\n");
        return 1;
    }
    bool in_random = strcmp(in, "/dev/urandom") == 0;
    bool in_zero = strcmp(in, "/dev/zero") == 0;
    bool out_null = strcmp(out, "/dev/null") == 0;
    if ((in_random || in_zero) && count == 0) {
        printf("count= is required when reading from %s\n", in);
        return 1;
    }
    res_id_t res = res_for_path(in_random || in_zero ? out : in);
    if (res != RES_MAX && !res_lock(res, "dd")) {
        return 1;
    }
    uint8_t *buf = heap_caps_malloc(bs, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!buf) {
        buf = heap_caps_malloc(bs, MALLOC_CAP_SPIRAM);
    }
    FILE *fin = NULL;
    FILE *fout = NULL;
    int ret = 1;
    uint64_t bytes = 0;
    size_t bad_words = 0;
    uint32_t seed_in = DD_SEED;
    uint32_t seed_out = DD_SEED;
    bool ok = false;
    if (!buf) {
        printf("no memory for bs=%u\n", (unsigned)bs);
        goto done;
    }
    if (!in_random && !in_zero && !(fin = fopen(in, "rb"))) {
        printf("cannot open %s\n", in);
        goto done;
    }
    if (!out_null && !(fout = fopen(out, "wb"))) {
        printf("cannot create %s\n", out);
        goto done;
    }
    int64_t t0 = esp_timer_get_time();
    for (size_t blk = 0; count == 0 || blk < count; blk++) {
        size_t n = bs;
        if (in_random) {
            prng_fill(buf, bs, &seed_in);
        } else if (in_zero) {
            memset(buf, 0, bs);
        } else {
            n = fread(buf, 1, bs, fin);
            if (n == 0) {
                break;
            }
        }
        if (out_null) {
            bad_words += prng_verify(buf, n, &seed_out);
        } else if (fwrite(buf, 1, n, fout) != n) {
            printf("write error after %llu bytes\n", (unsigned long long)bytes);
            goto done;
        }
        bytes += n;
        if (job_should_stop()) {
            break;
        }
    }
    if (fout) {
        fflush(fout);
        fsync(fileno(fout));
    }
    int64_t us = esp_timer_get_time() - t0;
    double mb_s = us > 0 ? bytes / (us / 1e6) / (1024.0 * 1024.0) : 0.0;
    /* Verification only makes sense for data written from /dev/urandom. */
    ok = !(out_null && bad_words);
    if (!job_is_background()) {
        printf("%llu bytes, %.3f s, %.2f MB/s%s\n", (unsigned long long)bytes, us / 1e6, mb_s,
               out_null ? (bad_words ? "  VERIFY FAILED" : "  verify ok") : "");
        if (out_null && bad_words) {
            printf("%u mismatching words (file not written by dd if=/dev/urandom?)\n", (unsigned)bad_words);
        }
        if (res != RES_MAX) {
            char item[32], value[24];
            const char *dev = res == RES_SD ? "sd" : "usb";
            if (res == RES_SD) {
                snprintf(item, sizeof(item), "%s.%s.%dM", dev, out_null ? "read" : "write", s_sd_freq_mhz);
            } else {
                snprintf(item, sizeof(item), "%s.%s", dev, out_null ? "read" : "write");
            }
            snprintf(value, sizeof(value), "%.2f MB/s", mb_s);
            report_set(item, ok ? REPORT_PASS : REPORT_FAIL, value, (out_null && bad_words) ? "verify failed" : NULL);
        }
    }
    job_report(ok, bytes);
    ret = ok ? 0 : 1;
done:
    if (fin) {
        fclose(fin);
    }
    if (fout) {
        fclose(fout);
    }
    free(buf);
    if (res != RES_MAX) {
        res_unlock(res);
    }
    return ret;
}

void register_storage_commands(void)
{
    const esp_console_cmd_t cmds[] = {
        { .command = "mount",  .help = "mount -t sd [-o freq=20|40] /sdcard | mount -t usb /usb", .func = cmd_mount },
        { .command = "umount", .help = "umount /sdcard | umount /usb", .func = cmd_umount },
        { .command = "df",     .help = "Show mounted volumes", .func = cmd_df },
        { .command = "ls",     .help = "List a directory. Usage: ls [/ | /sdcard | /usb | /gpio20]", .func = cmd_ls },
        { .command = "dd",     .help = "Copy/verify data. dd if=/dev/urandom of=/sdcard/t.bin bs=64k count=512 ; dd if=/sdcard/t.bin of=/dev/null", .func = cmd_dd },
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }
}
