# ESP32-P4-WIFI6-DB-POE-ETH BSP API

[English](API.md) | 简体中文

完整板级 API 的头文件：

```c
#include "bsp/esp-bsp.h"
```

板卡专用头文件为 `bsp/esp32_p4_wifi6_db_poe_eth.h`。

## 功能

| 功能 | 宏 | 值 |
| --- | --- | ---: |
| 显示 | `BSP_CAPS_DISPLAY` | 1 |
| 触摸 | `BSP_CAPS_TOUCH` | 1 |
| 音频 | `BSP_CAPS_AUDIO` | 1 |
| 扬声器 | `BSP_CAPS_AUDIO_SPEAKER` | 1 |
| 麦克风 | `BSP_CAPS_AUDIO_MIC` | 1 |
| SD 卡 | `BSP_CAPS_SDCARD` | 1 |
| 摄像头 | `BSP_CAPS_CAMERA` | 1 |
| RTC | `BSP_CAPS_RTC` | 0 |
| 以太网 | `BSP_CAPS_ETHERNET` | 1 |
| Wi-Fi 协处理器 | `BSP_CAPS_WIFI` | 1 |

## 以太网和 Wi-Fi

- `bsp_eth_init(&handle)` 安装开发板的 RMII MAC 和通用 PHY 驱动。
- `bsp_eth_deinit(handle)` 在应用停止驱动并释放 netif 引用后卸载 MAC/PHY。
- `bsp/ethernet.h` 提供以太网 API 和引脚定义，不依赖显示头文件。
- 应用负责以太网 netif、事件循环、DHCP 和桥接策略。
- `BSP_WIFI_*` 描述 ESP32-C5 的 SDIO 连接，Hosted 配置由应用管理。

## 板卡和 I2C

- `bsp_get_header_gpios()` 返回扩展排针 GPIO 数组。
- `bsp_i2c_init()` 初始化 GPIO7/GPIO8。
- `bsp_i2c_get_handle()` 返回共享 I2C 主机总线句柄。
- `bsp_i2c_deinit()` 在没有设备继续使用总线时释放它。

ES8311、GT911、显示背光控制器和摄像头 SCCB 共用该总线。

## ES8311 音频

- `bsp_audio_init()` 创建全双工 I2S 通道。
- `bsp_audio_codec_speaker_init()` 创建 ES8311 播放设备。
- `bsp_audio_codec_microphone_init()` 创建 ES8311 单麦克风采集设备。

I2S 引脚为 MCLK GPIO13、SCLK GPIO12、LRCK GPIO10、DSDIN GPIO9、ASDOUT GPIO11。

## 存储

- `bsp_sdcard_mount()` 和 `bsp_sdcard_unmount()` 使用开发板的 4-bit SDMMC 连接。
- `bsp_sdcard_sdmmc_mount()` 接受调用方提供的挂载点、主机或槽位配置。
- `bsp_sdcard_get_handle()` 返回已挂载的 SD 卡句柄。
- `bsp_spiffs_mount()` 和 `bsp_spiffs_unmount()` 提供可选的 SPIFFS 辅助接口。

## MIPI-DSI 显示

- `bsp_display_new()` 创建由 `BSP_LCD_TYPE` 选择的面板：JD9365 800x1280（8/10.1 英寸）、ILI9881C 720x1280（7 英寸）或 HX8394 720x1280（5 英寸）。
- `bsp_display_new_with_handles()` 同时返回 DSI 总线句柄。
- `bsp_display_delete()` 释放面板、DSI 总线、DSI PHY LDO 和背光设备。
- `bsp_display_brightness_set()` 向 I2C 地址 `0x45` 的寄存器 `0x96` 写入 8 位亮度值。
- `bsp_display_brightness_get()` 返回最近一次成功设置的亮度百分比。
- `bsp_display_backlight_on()` 和 `bsp_display_backlight_off()` 分别设置 100% 和 0%。

## GT911 触摸

- `bsp_touch_new()` 在 GPIO7/GPIO8 共享 I2C 总线上创建 GT911。
- 初始化会依次尝试 `0x5D` 和 `0x14` 两个 GT911 地址。
- `BSP_LCD_TOUCH_RST` 和 `BSP_LCD_TOUCH_INT` 为 `GPIO_NUM_NC`，触摸数据通过 I2C 轮询。
- `bsp_touch_delete()` 释放触摸控制器和面板 I/O 句柄。
- `bsp_touch_get_shared()` 返回 BSP 持有的 GT911 句柄，不转移所有权；只有在 LVGL adapter 暂停时才能直接读取。

启用 LVGL 支持时：

- `bsp_display_start()` 初始化显示、GT911 和 LVGL adapter。
- `bsp_display_start_with_config()` 接受旋转、防撕裂和触摸坐标变换配置。
- `bsp_display_stop()` 停止 LVGL 并释放显示资源。
- `bsp_display_get_input_dev()` 返回已注册的 LVGL 触摸输入设备。
- `bsp_display_lock()` 和 `bsp_display_unlock()` 保护 LVGL 访问。
- `bsp_display_set_dummy_draw()`、`bsp_display_get_free_frame_buffer()` 和 `bsp_display_flush_frame_buffer()` 支持直接写入帧缓冲。

## USB 和 CSI

- `bsp_usb_host_start()` 和 `bsp_usb_host_stop()` 管理 USB Host 库。
- `bsp_camera_start()` 使用共享 I2C 总线作为摄像头 SCCB 控制器，初始化 MIPI-CSI 路径。

更多接线、配置和生命周期示例见[组件说明](README_ZH.md)。
