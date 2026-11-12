# Wi-Fi Station

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过板载 ESP32-C5 和 ESP-Hosted SDIO 传输，让 ESP32-P4 Host 连接 Wi-Fi 接入点。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- Wi-Fi 接入点

## 怎么运行

先向 ESP32-C5 烧录兼容的 ESP-Hosted 固件，再在 `Example Configuration` 中设置 SSID 和密码，随后编译 Host 应用。

```bash
idf.py set-target esp32p4 menuconfig
idf.py build flash monitor
```

## 预期现象

连接成功后，日志先输出 `got ip:`，再输出 `connected to ap SSID:`。

## 常见问题

默认 SSID 和密码分别为 `myssid`、`mypassword`，烧录前需要修改；同时确认 ESP32-C5 固件与 Host 端 ESP-Hosted 配置兼容。
