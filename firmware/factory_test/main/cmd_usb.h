#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Install USB Host + MSC driver (if needed), wait for a mass-storage device and
 * mount its FAT volume at `path`. */
esp_err_t usb_msc_mount(const char *path, uint32_t timeout_ms);
esp_err_t usb_msc_umount(void);
bool usb_msc_is_mounted(void);

/* lsusb */
void register_usb_commands(void);

#ifdef __cplusplus
}
#endif
