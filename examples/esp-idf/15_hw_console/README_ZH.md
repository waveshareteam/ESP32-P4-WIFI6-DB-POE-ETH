# 硬件控制台

[English](README.md) | [简体中文](README_ZH.md)

## 功能简介

这是一个通过串口命令行操作 ESP32-P4-WIFI6-DB-POE-ETH 开发板的示例，提供系统信息、I2C、SD 卡和 USB 存储、Wi-Fi、显示与触摸、摄像头、音频及 GPIO 等命令。外设会在执行对应命令时初始化。

## 硬件

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 使用显示和触摸命令时，需要支持的 MIPI-DSI 屏幕（5、7、8 或 10.1 英寸）
- 使用相应命令时，需要 microSD 卡、USB 存储设备、OV5647 摄像头或扬声器
- 使用 Wi-Fi 吞吐命令时，需要安装 iPerf 2.x 的电脑

## 编译与运行

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py set-target esp32p4
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth
idf.py build flash monitor
```

在串口控制台输入 `help` 查看可用命令。命令格式和使用示例见[命令使用说明](docs/command-usage_ZH.md)。

使用其他屏幕时，使用屏幕对应的 amendment 重新运行 Board Manager，例如：

```bash
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth -a ../esp32_p4_dsi_overlay/7_dsi_touch_c
```

可选项有 `5_dsi_touch_a`、`7_dsi_touch_c`、`8_dsi_touch_a`、`10_1_dsi_touch_a`。
