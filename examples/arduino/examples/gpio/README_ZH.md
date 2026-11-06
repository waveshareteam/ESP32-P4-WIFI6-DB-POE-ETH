# gpio

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 Arduino-ESP32 的 `Console` 库，在串口监视器中交互读取、写入和配置 GPIO。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- USB 数据线

## 怎么运行

1. 在开发板管理器中安装 `esp32`（Arduino-ESP32），版本见仓库 README。
2. 按[板卡定义安装说明](../../libraries/board_config/README.md)添加开发板。
3. 把项目文件夹位置设为 `examples/arduino/`，或把 `examples/arduino/libraries/` 复制到项目文件夹下的 `libraries` 文件夹。
4. 选择 `Waveshare ESP32-P4-WIFI6-DB-POE-ETH`。
5. 打开 `gpio.ino` 并上传，然后以 115200 波特率打开串口监视器。

把行结束符设为 `Newline`，然后输入 `gpio mode 2 out`、`gpio write 2 1` 等命令。输入 `help` 查看全部命令。

## 预期现象

运行 `gpio mode 2 out` 后打印 `GPIO 2 mode set to out`，`gpio read 2` 打印当前电平。

## 常见问题

运行 `gpio write` 前先运行 `gpio mode`。板载外设工作时，不要使用它们占用的 GPIO。
