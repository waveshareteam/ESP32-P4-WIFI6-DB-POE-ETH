#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_vfs.h"
#include "esp_gpio_vfs.h"

#define DEFAULT_BASE_PATH ""   /* default VFS: /gpio<N>/... */
#define MAX_PIN           64

enum { ATTR_VALUE = 0, ATTR_DIRECTION = 1, ATTR_COUNT };
static const char *const s_attr_name[ATTR_COUNT] = { "value", "direction" };

/* fd = pin * ATTR_COUNT + attr; each open file is read once (then EOF). */
#define FD_MAX (MAX_PIN * ATTR_COUNT)

static const char *TAG = "gpio_vfs";
static esp_gpio_vfs_config_t s_cfg;
static char s_base_path[ESP_VFS_PATH_MAX + 1];
static bool s_registered;
static bool s_fd_consumed[FD_MAX];
static bool s_is_output[MAX_PIN];

typedef struct {
    DIR dir;            /* must be first: the VFS layer fills dd_vfs_idx */
    int pin;            /* -1 = root listing */
    int index;
    struct dirent entry;
} gpio_dir_t;

static bool pin_listed(int pin)
{
    for (size_t i = 0; i < s_cfg.pin_count; i++) {
        if (s_cfg.pins[i] == pin) {
            return true;
        }
    }
    return false;
}

static void set_direction(int pin, bool output)
{
    /* INPUT_OUTPUT so that reading `value` of an output pin returns the driven level */
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << pin,
        .mode = output ? GPIO_MODE_INPUT_OUTPUT : GPIO_MODE_INPUT,
        .pull_up_en = output ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE,
    };
    gpio_config(&cfg);
    s_is_output[pin] = output;
}

/* "/gpio20/value" -> pin 20, attr ATTR_VALUE. attr = -1 for "/gpio20", pin = -1 for "/" */
static int parse_path(const char *path, int *pin, int *attr)
{
    *pin = -1;
    *attr = -1;
    if (strcmp(path, "/") == 0 || path[0] == '\0') {
        return 0;
    }
    if (strncmp(path, "/gpio", 5) != 0) {
        return -1;
    }
    char *end;
    long n = strtol(path + 5, &end, 10);
    if (end == path + 5 || n < 0 || n >= MAX_PIN || !pin_listed((int)n)) {
        return -1;
    }
    *pin = (int)n;
    if (*end == '\0' || strcmp(end, "/") == 0) {
        return 0;
    }
    if (*end != '/') {
        return -1;
    }
    for (int a = 0; a < ATTR_COUNT; a++) {
        if (strcmp(end + 1, s_attr_name[a]) == 0) {
            *attr = a;
            return 0;
        }
    }
    return -1;
}

static int vfs_open(void *ctx, const char *path, int flags, int mode)
{
    int pin, attr;
    if (parse_path(path, &pin, &attr) != 0 || attr < 0) {
        errno = ENOENT;
        return -1;
    }
    int fd = pin * ATTR_COUNT + attr;
    s_fd_consumed[fd] = false;
    return fd;
}

static int vfs_close(void *ctx, int fd)
{
    return 0;
}

static ssize_t vfs_read(void *ctx, int fd, void *dst, size_t size)
{
    if (fd < 0 || fd >= FD_MAX) {
        errno = EBADF;
        return -1;
    }
    if (s_fd_consumed[fd]) {
        return 0;
    }
    int pin = fd / ATTR_COUNT;
    int attr = fd % ATTR_COUNT;
    char text[8];
    if (attr == ATTR_VALUE) {
        snprintf(text, sizeof(text), "%d\n", gpio_get_level(pin));
    } else {
        snprintf(text, sizeof(text), "%s\n", s_is_output[pin] ? "out" : "in");
    }
    size_t n = strlen(text);
    if (n > size) {
        n = size;
    }
    memcpy(dst, text, n);
    s_fd_consumed[fd] = true;
    return (ssize_t)n;
}

static ssize_t vfs_write(void *ctx, int fd, const void *data, size_t size)
{
    if (fd < 0 || fd >= FD_MAX) {
        errno = EBADF;
        return -1;
    }
    int pin = fd / ATTR_COUNT;
    int attr = fd % ATTR_COUNT;
    char text[8] = {0};
    size_t n = size < sizeof(text) - 1 ? size : sizeof(text) - 1;
    memcpy(text, data, n);
    for (char *p = text; *p; p++) {
        if (*p == '\n' || *p == '\r' || *p == ' ') {
            *p = '\0';
            break;
        }
    }
    if (s_cfg.blocked_cb) {
        const char *reason = s_cfg.blocked_cb(pin, s_cfg.blocked_ctx);
        if (reason) {
            ESP_LOGW(TAG, "gpio%d: %s", pin, reason);
            errno = EBUSY;
            return -1;
        }
    }
    if (attr == ATTR_VALUE) {
        if (strcmp(text, "0") != 0 && strcmp(text, "1") != 0) {
            errno = EINVAL;
            return -1;
        }
        if (!s_is_output[pin]) {
            set_direction(pin, true);
        }
        gpio_set_level(pin, text[0] - '0');
    } else {
        if (strcmp(text, "in") == 0) {
            set_direction(pin, false);
        } else if (strcmp(text, "out") == 0 || strcmp(text, "low") == 0) {
            set_direction(pin, true);
            gpio_set_level(pin, 0);
        } else if (strcmp(text, "high") == 0) {
            set_direction(pin, true);
            gpio_set_level(pin, 1);
        } else {
            errno = EINVAL;
            return -1;
        }
    }
    return (ssize_t)size;
}

static int vfs_stat(void *ctx, const char *path, struct stat *st)
{
    int pin, attr;
    if (parse_path(path, &pin, &attr) != 0) {
        errno = ENOENT;
        return -1;
    }
    memset(st, 0, sizeof(*st));
    st->st_mode = attr < 0 ? (S_IFDIR | 0755) : (S_IFREG | 0644);
    st->st_size = attr < 0 ? 0 : 4;
    return 0;
}

static DIR *vfs_opendir(void *ctx, const char *name)
{
    int pin, attr;
    if (parse_path(name, &pin, &attr) != 0 || attr >= 0) {
        errno = ENOENT;
        return NULL;
    }
    gpio_dir_t *d = calloc(1, sizeof(*d));
    if (!d) {
        errno = ENOMEM;
        return NULL;
    }
    d->pin = pin;
    return (DIR *)d;
}

static struct dirent *vfs_readdir(void *ctx, DIR *pdir)
{
    gpio_dir_t *d = (gpio_dir_t *)pdir;
    if (d->pin < 0) {
        if (d->index >= (int)s_cfg.pin_count) {
            return NULL;
        }
        d->entry.d_type = DT_DIR;
        snprintf(d->entry.d_name, sizeof(d->entry.d_name), "gpio%d", s_cfg.pins[d->index]);
    } else {
        if (d->index >= ATTR_COUNT) {
            return NULL;
        }
        d->entry.d_type = DT_REG;
        snprintf(d->entry.d_name, sizeof(d->entry.d_name), "%s", s_attr_name[d->index]);
    }
    d->entry.d_ino = d->index;
    d->index++;
    return &d->entry;
}

static int vfs_closedir(void *ctx, DIR *pdir)
{
    free(pdir);
    return 0;
}

esp_err_t esp_gpio_vfs_register(const esp_gpio_vfs_config_t *config)
{
    ESP_RETURN_ON_FALSE(config && config->pins && config->pin_count > 0, ESP_ERR_INVALID_ARG, TAG, "no pins");
    ESP_RETURN_ON_FALSE(!s_registered, ESP_ERR_INVALID_STATE, TAG, "already registered");
    for (size_t i = 0; i < config->pin_count; i++) {
        ESP_RETURN_ON_FALSE(config->pins[i] >= 0 && config->pins[i] < MAX_PIN, ESP_ERR_INVALID_ARG,
                            TAG, "pin %d out of range", config->pins[i]);
    }
    const char *base = config->base_path ? config->base_path : DEFAULT_BASE_PATH;
    ESP_RETURN_ON_FALSE(strlen(base) <= ESP_VFS_PATH_MAX, ESP_ERR_INVALID_ARG, TAG,
                        "base path longer than %d", ESP_VFS_PATH_MAX);
    s_cfg = *config;
    strlcpy(s_base_path, base, sizeof(s_base_path));
    memset(s_is_output, 0, sizeof(s_is_output));

    static const esp_vfs_t vfs = {
        .flags = ESP_VFS_FLAG_CONTEXT_PTR,
        .open_p = vfs_open,
        .close_p = vfs_close,
        .read_p = vfs_read,
        .write_p = vfs_write,
        .stat_p = vfs_stat,
        .opendir_p = vfs_opendir,
        .readdir_p = vfs_readdir,
        .closedir_p = vfs_closedir,
    };
    ESP_RETURN_ON_ERROR(esp_vfs_register(s_base_path, &vfs, NULL), TAG, "esp_vfs_register");
    s_registered = true;
    ESP_LOGI(TAG, "%u GPIO(s) mounted at %s/gpio<N>", (unsigned)s_cfg.pin_count, s_base_path);
    return ESP_OK;
}

esp_err_t esp_gpio_vfs_unregister(void)
{
    if (!s_registered) {
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(esp_vfs_unregister(s_base_path), TAG, "esp_vfs_unregister");
    s_registered = false;
    return ESP_OK;
}
