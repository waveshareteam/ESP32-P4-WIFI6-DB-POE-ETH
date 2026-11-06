# Wi-Fi Station

[English](README.md) | [简体中文](README_ZH.md)

## 功能

以 Wi-Fi Station 方式连接接入点。应用运行在 ESP32-P4 上，射频由 ESP32-C5 通过 ESP-Hosted SDIO 提供。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- Wi-Fi 接入点

## 怎么运行

先从 [`cp`](cp/README_ZH.md) 烧录 ESP32-C5 协处理器，再从本目录烧录 ESP32-P4 主机。在 menuconfig 的 `Example Configuration` 中设置 SSID 和密码。

```bash
cd cp
idf.py set-target esp32c5 build
idf.py -p <C5_PORT> flash
cd ..
idf.py set-target esp32p4 menuconfig
idf.py build
idf.py -p <P4_PORT> flash monitor
```

## 预期现象

ESP32-P4 日志打印 `got ip:` 和分配到的地址，然后打印 `connected to ap SSID:`。

## 常见问题

SSID 和密码默认是 `myssid` 和 `mypassword`，烧录前需要修改。[`cp_streaming`](cp_streaming/README_ZH.md) 是另一种协处理器构建。
