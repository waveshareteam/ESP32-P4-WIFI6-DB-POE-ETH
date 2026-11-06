# esp_gpio_vfs

Exposes a list of GPIOs as a Linux-sysfs-like tree on the ESP-IDF VFS so the
standard C file API (and console commands built on it such as `cat`, `echo ... >`
and `ls`) can drive and read pins:

```
/gpio20/value        read "0"/"1"; write "0"/"1" (switches the pin to output)
/gpio20/direction    read "in"/"out"; write "in"/"out"/"high"/"low"
```

By default the tree is registered with an empty base path, i.e. as the ESP-IDF
*default VFS*: any path no other prefix (`/sdcard`, `/usb`, ...) claims is
routed here, so `/gpio20/value` works directly and `ls /` lists the pins.

Only the pins passed at registration exist (no `export` step). An optional
callback can veto writes to a pin that is currently owned by another driver;
such writes fail with `EBUSY`.

```c
#include "esp_gpio_vfs.h"

static const int pins[] = { 2, 3, 4, 5 };

static const char *blocked(int pin, void *ctx)
{
    return (pin == 5 && sd_card_mounted) ? "SD power enable" : NULL;
}

esp_gpio_vfs_config_t cfg = {
    .pins = pins,
    .pin_count = sizeof(pins) / sizeof(pins[0]),
    .blocked_cb = blocked,
};
ESP_ERROR_CHECK(esp_gpio_vfs_register(&cfg));

FILE *f = fopen("/gpio2/value", "w");
fputs("1", f);
fclose(f);
```

`base_path` defaults to `""` (root). A prefix such as `/sys/class/gpio` may be
given instead; it must be at most `ESP_VFS_PATH_MAX` (15) characters.
