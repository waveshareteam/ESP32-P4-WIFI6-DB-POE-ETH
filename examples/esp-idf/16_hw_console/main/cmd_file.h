#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* cat <path> ; echo <text...> [> <path> | >> <path>] — work on any VFS path
 * (/gpio<N>/..., /sdcard/..., /usb/...) */
void register_file_commands(void);

#ifdef __cplusplus
}
#endif
