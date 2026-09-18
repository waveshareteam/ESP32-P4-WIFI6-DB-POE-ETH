#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* mount / umount / df / ls / dd (SD card and USB MSC share the file commands) */
void register_storage_commands(void);

bool storage_sd_is_mounted(void);   /* cmd_gpio uses this to protect GPIO45 (SD power enable) */
int storage_sd_freq_mhz(void);      /* 20 / 40, 0 when unmounted */

#ifdef __cplusplus
}
#endif
