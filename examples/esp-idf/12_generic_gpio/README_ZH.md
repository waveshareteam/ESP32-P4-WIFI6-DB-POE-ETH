# GPIO 监视器

[English](README.md) | [简体中文](README_ZH.md)

## 功能

在 LVGL 网格中实时显示扩展排针 GPIO 的电平，并可通过触摸屏切换输入和输出模式。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 与 Board Manager 板卡定义匹配的 MIPI-DSI 屏幕

## 怎么运行

屏幕配置来自 Board Manager 板卡定义。默认 GPIO 方向和上下拉模式在 `GPIO Monitor Example` 菜单中设置。

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

LCD 为每个排针 GPIO 显示一张卡片和当前电平，顶部开关用于切换全部 GPIO 的模式。

## 常见问题

切换到输出模式前，先断开接在扩展排针上的外部设备。
