# LVGL 性能测试

[English](README.md) | [简体中文](README_ZH.md)

## 功能

在 MIPI-DSI 屏幕上运行 LVGL benchmark 演示，用于测试显示和渲染性能。

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

背光点亮，演示依次运行各场景，串口日志打印 FPS 和渲染时间统计。

## 常见问题

本示例不初始化触摸，需要触摸输入时请使用 [12_generic_gpio](../12_generic_gpio/README_ZH.md)。
