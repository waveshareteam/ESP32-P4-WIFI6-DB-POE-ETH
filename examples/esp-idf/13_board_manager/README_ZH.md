# Board Manager 测试程序

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 ESP Board Manager 启动开发板的交互式控制台，用于运行 LCD、触摸、音频、摄像头、SD 卡和 GPIO 测试用例。本示例改编自 `esp_board_manager` 的测试程序，其中乐鑫的源码适用本目录的 [`LICENSE`](LICENSE)。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 支持的 MIPI-DSI 屏幕（5、7、8 或 10.1 英寸）

## 怎么运行

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py set-target esp32p4
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth
idf.py build flash monitor
```

默认屏幕是 10.1 英寸 DSI 屏。使用其他屏幕时，加上对应的 amend 目录（`5_dsi_touch_a`、`7_dsi_touch_c`、`8_dsi_touch_a`、`10_1_dsi_touch_a` 或 `10_1_dsi_touch_b`）：

```bash
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth -a ../esp32_p4_dsi_overlay/7_dsi_touch_c
```

## 预期现象

出现控制台提示符。依次运行 `bmgr init`、`case list`，再运行某个用例，例如 `case run lcd.pattern.checker`。

## 常见问题

运行 `idf.py build` 前至少要运行一次 `bmgr`，更换屏幕后需要重新运行。控制台使用 UART0，即 USB 下载口。
