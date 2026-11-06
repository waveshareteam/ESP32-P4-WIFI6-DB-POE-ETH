# 开发板检查

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过串口打印芯片、Flash、PSRAM 和堆内存信息。建议首先运行本示例，确认工具链、烧录和串口监视器工作正常。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- USB 数据线

## 怎么运行

```bash
idf.py set-target esp32p4 build flash monitor
```

## 预期现象

串口监视器显示 `ESP32-P4 Platform Board Check` 标题和开发板信息，之后每五秒打印一条 `alive` 日志。

## 常见问题

日志里的 Flash 大小是本示例 `sdkconfig.defaults` 配置的值，小于开发板实际的 32 MB。
