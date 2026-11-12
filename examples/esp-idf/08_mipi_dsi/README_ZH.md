# MIPI-DSI 显示

[English](README.md) | [简体中文](README_ZH.md)

## 功能

在屏幕上显示硬件彩条。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 与 Board Manager 板卡定义匹配的 MIPI-DSI 屏幕

## 怎么运行

屏幕配置来自 Board Manager 板卡定义。编译前需要生成板卡代码。

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

背光点亮并显示竖向彩条，日志打印 `Show color bar pattern drawn by hardware`。

## 常见问题

日志正常但屏幕不亮时，检查 FPC 方向和板卡显示配置。本板没有 LCD 复位 GPIO。
