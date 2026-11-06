# BLE 外设（最小示例）

[English](README.md) | [简体中文](README_ZH.md)

## 功能

最小 NimBLE 外设，通过 ESP-Hosted HCI 使用 ESP32-C5 的蓝牙控制器进行广播。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 带 BLE 扫描工具的手机或电脑

## 怎么运行

先从 [`cp`](cp/README_ZH.md) 烧录 ESP32-C5 控制器，再从本目录烧录 ESP32-P4 主机：

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

日志打印 `advertising as "esp-hosted-nim"`，可在 BLE 扫描工具中发现并连接该设备。

## 常见问题

NimBLE 只支持 BLE，不支持经典蓝牙。主机和协处理器必须使用相同的传输方式。
