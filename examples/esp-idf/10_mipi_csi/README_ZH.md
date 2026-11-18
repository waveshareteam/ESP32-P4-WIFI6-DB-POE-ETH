# MIPI-CSI 摄像头

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 `esp_video` 和 PPA 把 MIPI-CSI 摄像头的实时画面显示到 LCD。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 与 Board Manager 板卡定义匹配的 MIPI-DSI 屏幕
- OV5647 MIPI-CSI 摄像头

## 怎么运行

屏幕配置来自 Board Manager 板卡定义。使用其他传感器时，还需修改 `Espressif Camera Sensors Configurations` 中的设置。

连接其他屏幕时，将下方的 `idf.py bmgr` 命令替换为匹配的显示 amendment，例如：

```bash
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth -a ../esp32_p4_dsi_overlay/7_dsi_touch_c
```

可选项有 `5_dsi_touch_a`、`7_dsi_touch_c`、`8_dsi_touch_a`、`10_1_dsi_touch_a`。

```bash
idf.py set-target esp32p4
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth
idf.py menuconfig
idf.py build flash monitor
```

## 预期现象

背光点亮，LCD 显示摄像头实时画面，日志打印视频设备和帧尺寸。

## 常见问题

建议先运行 [08_mipi_dsi](../08_mipi_dsi/README_ZH.md) 确认屏幕正常。出现 `video cam open failed` 时，检查摄像头 FPC 方向和所选传感器。
