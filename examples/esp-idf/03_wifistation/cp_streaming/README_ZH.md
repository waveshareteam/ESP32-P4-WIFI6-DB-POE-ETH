# ESP32-C5 协处理器（Streaming）

[English](README.md) | [简体中文](README_ZH.md)

## 功能

[03_wifistation](../README_ZH.md) 使用的 ESP-Hosted 协处理器固件的 Streaming 版本。

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

传输方式要与 ESP32-P4 主机保持一致。
