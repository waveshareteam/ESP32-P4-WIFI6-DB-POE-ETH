#pragma once

#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Linux-sysfs-like GPIO tree on the ESP-IDF VFS:
 *
 *   /gpio<N>/value       read  "0" / "1"
 *                        write "0" / "1"  (the pin is switched to output)
 *   /gpio<N>/direction   read  "in" / "out"
 *                        write "in" / "out" / "high" / "low"
 *
 * With the default (empty) base path the tree is registered as the ESP-IDF
 * "default VFS": every path that no other VFS prefix (/sdcard, /usb, ...)
 * claims is routed here, so /gpio20/value works directly and `ls /` lists the
 * pins. A base path such as "/sys/class/gpio" can be given instead.
 * Only the pins listed in the config exist; everything else is ENOENT.
 * `ls`, `stat`, `cat` and `echo ... >` work through the normal C file API. */

/* Optional: return a reason string when `pin` must not be driven right now
 * (writes then fail with EBUSY), or NULL when it is free. */
typedef const char *(*esp_gpio_vfs_blocked_cb_t)(int pin, void *ctx);

typedef struct {
    const char *base_path;                 /* NULL or "" = root (/gpio<N>/...); else e.g. "/sys/class/gpio" (max 15 chars, no trailing slash) */
    const int *pins;                       /* GPIO numbers to expose; the array must stay valid */
    size_t pin_count;
    esp_gpio_vfs_blocked_cb_t blocked_cb;  /* may be NULL */
    void *blocked_ctx;
} esp_gpio_vfs_config_t;

esp_err_t esp_gpio_vfs_register(const esp_gpio_vfs_config_t *config);
esp_err_t esp_gpio_vfs_unregister(void);

#ifdef __cplusplus
}
#endif
