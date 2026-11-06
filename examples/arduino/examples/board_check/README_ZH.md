# board_check

[English](README.md) | [简体中文](README_ZH.md)

## 功能

打印芯片、CPU、Flash、PSRAM 和堆内存信息。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- USB 数据线

## 怎么运行

1. 在开发板管理器中安装 `esp32`（Arduino-ESP32），版本见仓库 README。
2. 按[板卡定义安装说明](../../libraries/board_config/README.md)添加开发板。
3. 把项目文件夹位置设为 `examples/arduino/`，或把 `examples/arduino/libraries/` 复制到项目文件夹下的 `libraries` 文件夹。
4. 选择 `Waveshare ESP32-P4-WIFI6-DB-POE-ETH`。
5. 打开 `board_check.ino` 并上传，然后以 115200 波特率打开串口监视器。

## 预期现象

串口监视器显示板名，然后显示芯片、Flash、PSRAM 和堆内存信息。

## 常见问题

暂无。
