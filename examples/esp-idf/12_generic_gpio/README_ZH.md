# GPIO 监视器

[English](README.md) | [简体中文](README_ZH.md)

## 功能

在 LVGL 网格中实时显示扩展排针 GPIO 的电平，并可通过触摸屏切换输入和输出模式。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 支持的 MIPI-DSI 屏幕（5、7、8 或 10.1 英寸）

## 怎么运行

在 `Component config > Board Support Package (ESP32-P4-WIFI6-DB-POE-ETH) > Display > Select LCD type` 中选择实际连接的屏幕，默认是 10.1 英寸屏。默认 GPIO 方向和上下拉模式在 `GPIO Monitor Example` 菜单中设置。

```bash
idf.py set-target esp32p4 menuconfig
idf.py build flash monitor
```

## 预期现象

LCD 为每个排针 GPIO 显示一张卡片和当前电平，顶部开关用于切换全部 GPIO 的模式。

## 常见问题

切换到输出模式前，先断开接在扩展排针上的外部设备。
