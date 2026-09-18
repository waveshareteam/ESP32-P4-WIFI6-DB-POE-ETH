#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* v4l2-ctl --stream | v4l2-ctl --stop */
void register_camera_commands(void);

#ifdef __cplusplus
}
#endif
