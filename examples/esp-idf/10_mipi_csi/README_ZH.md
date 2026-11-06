# MIPI-CSI 摄像头

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 `esp_video` 和 PPA 把 MIPI-CSI 摄像头的实时画面显示到 LCD。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 支持的 MIPI-DSI 屏幕（5、7、8 或 10.1 英寸）
- OV5647 MIPI-CSI 摄像头

## 怎么运行

在 `Component config > Board Support Package (ESP32-P4-WIFI6-DB-POE-ETH) > Display > Select LCD type` 中选择实际连接的屏幕，默认是 10.1 英寸屏。使用其他传感器时，还需修改 `Espressif Camera Sensors Configurations` 中的设置。

```bash
idf.py set-target esp32p4 menuconfig
idf.py build flash monitor
```

## 预期现象

背光点亮，LCD 显示摄像头实时画面，日志打印视频设备和帧尺寸。

## 常见问题

建议先运行 [08_mipi_dsi](../08_mipi_dsi/README_ZH.md) 确认屏幕正常。出现 `video cam open failed` 时，检查摄像头 FPC 方向和所选传感器。
