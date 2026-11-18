# mipi_csi

[English](README.md) | [简体中文](README_ZH.md)

## 功能

把 OV5647 摄像头画面显示到 MIPI-DSI 屏幕。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 支持的 MIPI-DSI 屏幕（5、7、8 或 10.1 英寸）
- OV5647 MIPI-CSI 摄像头

## 怎么运行

1. 在开发板管理器中安装 `esp32`（Arduino-ESP32），版本见仓库 README。
2. 按[板卡定义安装说明](../../libraries/board_config/README.md)添加开发板。
3. 把项目文件夹位置设为 `examples/arduino/`，或把 `examples/arduino/libraries/` 复制到项目文件夹下的 `libraries` 文件夹。
4. 选择 `Waveshare ESP32-P4-WIFI6-DB-POE-ETH`。
5. 在 `lcd_panel.h` 中通过 `LCD_PANEL_TYPE` 选择实际屏幕，默认是 JD9365 10.1 英寸屏。
6. 打开 `mipi_csi.ino` 并上传，然后以 115200 波特率打开串口监视器。

## 预期现象

屏幕显示摄像头画面。

## 常见问题

`ESP_Video` 必须启用 `CONFIG_ESP_VIDEO_ENABLE_MIPI_CSI_VIDEO_DEVICE=y`，使用自定义 Arduino-ESP32 构建时要保留这项配置。
