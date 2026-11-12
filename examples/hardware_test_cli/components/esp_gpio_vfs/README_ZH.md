# esp_gpio_vfs

[English](README.md) | 简体中文

该组件在 ESP-IDF VFS 上提供类似 Linux sysfs 的 GPIO 文件树，标准 C 文件 API
以及 `cat`、`echo ... >`、`ls` 等控制台命令都可以通过它读写 GPIO：

```text
/gpio20/value        读取或写入 0/1，写入时自动切换为输出
/gpio20/direction    读取或写入 in/out/high/low
```

默认情况下文件树注册在空基础路径，也就是 ESP-IDF 的默认 VFS。没有其他前缀
处理的路径会路由到这里，因此 `/gpio20/value` 和 `ls /` 可以直接使用。

注册时传入的 GPIO 才会出现在文件树中，不需要执行 `export`。可选的回调可以
拒绝正在被其他驱动使用的 GPIO 写入，此时写操作返回 `EBUSY`。

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
```

`base_path` 默认为空字符串，也可以设置为 `/sys/class/gpio` 等前缀；长度不能
超过 `ESP_VFS_PATH_MAX`（15）个字符。
