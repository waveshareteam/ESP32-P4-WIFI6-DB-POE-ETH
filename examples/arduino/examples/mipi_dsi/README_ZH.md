# mipi_dsi

[English](README.md) | [简体中文](README_ZH.md)

## 功能

在 MIPI-DSI 屏幕上循环显示色条，并测试 I2C 背光控制器。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 支持的 MIPI-DSI 屏幕（5、7、8 或 10.1 英寸）

## 怎么运行

1. 在开发板管理器中安装 `esp32`（Arduino-ESP32），版本见仓库 README。
2. 按[板卡定义安装说明](../../libraries/board_config/README.md)添加开发板。
3. 把项目文件夹位置设为 `examples/arduino/`，或把 `examples/arduino/libraries/` 复制到项目文件夹下的 `libraries` 文件夹。
4. 选择 `Waveshare ESP32-P4-WIFI6-DB-POE-ETH`。
5. 安装 `ESP32_Display_Panel` 1.0.4、`ESP32_IO_Expander` 1.1.1 和 `esp-lib-utils` 0.2.3，然后在 `esp_panel_drivers_conf.h` 中启用一个本地 LCD 驱动，默认是 JD9365 10.1 英寸屏。
6. 打开 `mipi_dsi.ino` 并上传，然后以 115200 波特率打开串口监视器。

## 预期现象

屏幕循环显示水平和垂直色条。

## 常见问题

看到 `LCD: start complete` 但没有图像时，检查驱动和时序是否与屏幕匹配。背光初始化失败时，检查 I2C 地址 `0x45` 的设备。
