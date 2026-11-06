# MIPI-DSI 显示

[English](README.md) | [简体中文](README_ZH.md)

## 功能

在屏幕上显示硬件彩条。可在 menuconfig 中切换为白屏测试、触摸画点测试或深度睡眠测试。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 支持的 MIPI-DSI 屏幕（5、7、8 或 10.1 英寸）

## 怎么运行

在 `Component config > Board Support Package (ESP32-P4-WIFI6-DB-POE-ETH) > Display > Select LCD type` 中选择实际连接的屏幕，默认是 10.1 英寸屏。

```bash
idf.py set-target esp32p4 menuconfig
idf.py build flash monitor
```

## 预期现象

背光点亮并显示竖向彩条，日志打印 `Show color bar pattern drawn by hardware`。

## 常见问题

日志正常但屏幕不亮时，检查 FPC 方向和屏幕型号选择。本板没有 LCD 复位 GPIO。
