## USB 扩展屏示例

[English](README.md)

使用 [LaunchPad](https://espressif.github.io/esp-launchpad/?flashConfigURL=https://dl.espressif.com/AE/esp-iot-solution/config.toml) 烧录该示例

USB 扩展屏示例可以将 ESP32-P4-WIFI6-DB-POE-ETH 作为一块 Windows 副屏。板级外设由 [ESP Board Manager](https://github.com/espressif/esp-board-manager) 描述和初始化。

本示例按 Board Manager 设备名获取板级外设，需要提供帧缓冲格式为 RGB565 或 RGB888 的横屏 `display_lcd` 设备；触摸使用 `lcd_touch`，USB 音频使用 `audio_dac` 和 `audio_adc`。Waveshare 板卡默认配置为 800×1280 竖屏，本示例不支持该方向；编译前需要通过 `-a` 选择受支持的横屏配置。背光控制仅支持使用 LEDC 控制设备的 `lcd_brightness`，不处理 custom 类型的亮度设备。10.1B 屏幕不支持。

支持以下功能：

* 支持由 Board Manager 配置的 RGB565 或 RGB888 横屏
* 支持最多五点触摸
* 支持通过板载音频编解码器输入和输出音频

## 所需硬件

1. ESP32-P4-WIFI6-DB-POE-ETH 开发板。
2. 受支持的横屏 MIPI-DSI 屏幕及对应的 Board Manager 显示 amendment。默认 800×1280 竖屏配置和 10.1B 屏幕不受本示例支持。
3. 启用 USB 音频时需要扬声器。

## 硬件连接

* 连接

    1. 将开发板上的高速 USB 口连接到 PC 上

## 编译和烧录

### 设备端

构建项目并将其烧录到板子上，然后运行监控工具以查看串行输出：

1. 运行 `. ./export.sh` 以设置 IDF 环境。

2. 在当前 ESP-IDF Python 环境中安装一次 Board Manager action helper：

   ```bash
   pip install esp-bmgr-assist
   ```

3. 从仓库根目录进入本示例目录，并生成产品板卡配置：

   ```bash
   cd examples/esp-idf/11_usb_extend_screen
   idf.py bmgr -b esp32_p4_wifi6_db_poe_eth
   ```

   `bmgr` 会自动选择目标芯片，并生成 `components/gen_bmgr_codes`，不需要再单独执行 `idf.py set-target`。

4. 使用对应的 Board Manager amendment 选择受支持的横屏配置：

   ```bash
   idf.py bmgr -b esp32_p4_wifi6_db_poe_eth -a ../esp32_p4_dsi_overlay/7_dsi_touch_c
   ```

   该命令应用 7-DSI-TOUCH-C 的 1024×600 横屏配置。10.1B 屏幕不受支持。

5. 运行 `idf.py -p PORT flash monitor` 来构建、烧录并监控项目。

（要退出串行监视器，请按 `Ctrl-]`。）

请参阅《入门指南》了解配置和使用 ESP-IDF 构建项目的所有步骤。

> 注意：首次配置/编译会下载托管组件，请确保网络可用。

### PC 端

准备工作，请参考 [windows_driver](./windows_driver/README_ZH.md)

![Demo](https://dl.espressif.com/AE/esp-iot-solution/p4_usb_extern_screen.gif)

## 其他问题

### 触摸屏控制的不是设备端的屏幕

* 在控制面板中选择 `平板电脑设置`

* 在配置栏中选择 `设置`

* 按照提示选择扩展屏

### 调高/调低 JPEG 的图片质量

* 修改 `CONFIG_USB_EXTEND_SCREEN_JPEG_QUALITY`，数字越大质量越高，同样的一帧图像占用内存更多。

### 修改副屏分辨率

* 分辨率以及 RGB565/RGB888 帧缓冲格式直接来自所选板卡的 `display_lcd` 配置，并同时用于生成 USB vendor 字符串、HID 触摸坐标范围、帧校验和 JPEG 输出缓冲区。
* 更换屏幕时，请选择受支持的横屏 amendment 并重新生成 Board Manager 配置。10.1B 屏幕不受支持。

Note: 目前驱动不支持竖屏的屏幕，请使用硬件上为横屏的屏幕。

### ESP32-P4 芯片版本与 JPEG 对齐

* ESP-IDF 对 ESP32-P4 `<3.0` 和 `>=3.0` 生成的固件互不兼容。Board Manager 负责选择板卡和目标芯片，但不代替芯片版本选择。使用 `<3.0` 芯片时，请在 `idf.py menuconfig` 中启用 `CONFIG_ESP32P4_SELECTS_REV_LESS_V3`，然后重新完整编译；不要把这一选项写进通用板卡 defaults。
* 硬件 JPEG 解码输出会按输入 JPEG 的 MCU 对齐：YUV444 为 8×8、YUV422 为 16×8、YUV420 为 16×16。示例会解析每帧 JPEG 头、预留最坏情况下的 16×16 对齐空间，并在送屏前移除行尾填充，因此像 1024×600 这样的可见分辨率不需要改成 1024×608。
* `<3.0` 芯片还限制部分 YUV 到 YUV 的格式转换；本示例直接解码为屏幕配置的 RGB565 或 RGB888 帧缓冲格式，不使用这些受限组合。

### 修改图像输出帧率

* 修改 `CONFIG_USB_EXTEND_SCREEN_MAX_FPS`，降低该值可以有效的减少 USB 带宽，当 USB AUDIO 音频卡顿时，可以适当减少此值。

### 修改一帧图像的最大值

* 修改 `CONFIG_USB_EXTEND_SCREEN_FRAME_LIMIT_B`，可以限制 PC 驱动传来的图像最大长度。

### 不启用触摸屏功能

* 修改 `CONFIG_HID_TOUCH_ENABLE` 为 `n`
* 只有所选板卡提供 `lcd_touch` 时，此选项才可启用。

### 不启用音频功能

* 修改 `CONFIG_UAC_AUDIO_ENABLE` 为 `n`
* 只有所选板卡提供 Board Manager 音频编解码设备时，此选项才可启用。

Note: 当只启用副屏功能，请将 PID 修改为 `0x2987`
