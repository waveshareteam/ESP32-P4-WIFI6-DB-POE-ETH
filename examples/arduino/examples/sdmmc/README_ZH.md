# sdmmc

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 SDMMC 在 microSD 卡上读写测试文件。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- microSD 卡

## 怎么运行

1. 在开发板管理器中安装 `esp32`（Arduino-ESP32），版本见仓库 README。
2. 按[板卡定义安装说明](../../libraries/board_config/README.md)添加开发板。
3. 把项目文件夹位置设为 `examples/arduino/`，或把 `examples/arduino/libraries/` 复制到项目文件夹下的 `libraries` 文件夹。
4. 选择 `Waveshare ESP32-P4-WIFI6-DB-POE-ETH`。
5. 打开 `sdmmc.ino` 并上传，然后以 115200 波特率打开串口监视器。

## 预期现象

串口监视器显示卡信息和从测试文件读回的内容。

## 常见问题

示例会在卡的根目录写入 `/arduino_test.txt`。
