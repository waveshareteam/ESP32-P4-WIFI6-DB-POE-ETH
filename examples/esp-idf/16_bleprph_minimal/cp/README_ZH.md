# ESP32-C5 协处理器（蓝牙）

[English](README.md) | [简体中文](README_ZH.md)

## 功能

板载 ESP32-C5 的 ESP-Hosted 固件，仅提供蓝牙控制器，供 [16_bleprph_minimal](../README_ZH.md) 使用。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 可连接 ESP32-C5 下载口

## 怎么运行

```bash
idf.py set-target esp32c5 build
idf.py -p <C5_PORT> flash
```

## 预期现象

固件通过 SDIO 等待 ESP32-P4 主机连接。

## 常见问题

此构建关闭了 Wi-Fi，Wi-Fi 示例请使用 `15_wifi_iperf/cp` 的固件。
