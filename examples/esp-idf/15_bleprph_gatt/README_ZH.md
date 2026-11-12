# BLE Peripheral GATT

[English](README.md) | [简体中文](README_ZH.md)

## 功能

ESP32-P4 Host 通过 ESP-Hosted HCI 使用 ESP32-C5 蓝牙控制器运行 NimBLE 外设。示例提供一个自定义 GATT 服务，包含读写特征和通知特征。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 支持 BLE 扫描的手机或电脑

## 怎么运行

先向 ESP32-C5 烧录兼容的 ESP-Hosted 蓝牙固件；可在 `BLE Peripheral GATT config` 中设置广播名称和通知间隔，再编译并烧录 Host 应用。

```bash
idf.py set-target esp32p4 menuconfig
idf.py build flash monitor
```

## 预期现象

日志输出包含配置名称的 `advertising as`。BLE 客户端可读写自定义值、订阅通知，并周期性收到计数器值。

## 常见问题

确认 NimBLE 已启用，且 ESP32-C5 运行与 Host 兼容的 Hosted 蓝牙固件。默认广播名称为 `esp-hosted-nim-gatt`。
