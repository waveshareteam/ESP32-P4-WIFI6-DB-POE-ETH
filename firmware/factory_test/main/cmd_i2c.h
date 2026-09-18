#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* i2cconfig / i2cdetect / i2cget / i2cset / i2cdump on the shared GPIO7/GPIO8 bus */
void register_i2c_commands(void);

#ifdef __cplusplus
}
#endif
