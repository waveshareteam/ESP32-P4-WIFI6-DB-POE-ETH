# BLE Peripheral GATT

[English](README.md) | [简体中文](README_ZH.md)

## 功能

ESP32-P4 Host 通过 ESP-Hosted HCI 使用 ESP32-C5 蓝牙控制器运行 NimBLE 外设。示例提供一个自定义 GATT 服务，包含读写特征和通知特征。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 支持 BLE 扫描的手机或电脑

## 怎么运行

先向 ESP32-C5 烧录兼容的 ESP-Hosted 蓝牙固件；可在 `BLE Peripheral GATT config` 中设置通知间隔，再编译并烧录 Host 应用。

```bash
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth
idf.py set-target esp32p4 menuconfig
idf.py build flash monitor
```

## 预期现象

日志输出包含动态设备名的 `advertising as`。设备名格式为 `esp-hosted-nim-` 加标准 MAC 地址最后两字节对应的 4 个小写十六进制字符。BLE 客户端可读写自定义值、订阅通知，并周期性收到计数器值。

## 常见问题

确认 NimBLE 已启用，且 ESP32-C5 运行与 Host 兼容的 Hosted 蓝牙固件。广播名称在 NimBLE Host 与 C5 控制器同步后，根据 C5 的 BLE identity MAC 地址动态生成。
