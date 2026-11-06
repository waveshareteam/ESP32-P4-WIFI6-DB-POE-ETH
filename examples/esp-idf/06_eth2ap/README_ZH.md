# 以太网转 Wi-Fi AP

[English](README.md) | [简体中文](README_ZH.md)

## 功能

在 MAC 层桥接以太网和 Wi-Fi 接入点，由 ESP32-C5 提供射频。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 网线和路由器或交换机
- Wi-Fi 客户端设备

## 怎么运行

ESP32-C5 需要先烧录 [03_wifistation/cp](../03_wifistation/cp/README_ZH.md) 的协处理器固件。

```bash
idf.py set-target esp32p4 build flash monitor
```

## 预期现象

以太网连接后出现 `eth2ap` 接入点，连接它的设备可以访问有线网络。

## 常见问题

默认 SSID 和密码是 `eth2ap` 和 `12345678`，可在 `Example Configuration` 中修改。
