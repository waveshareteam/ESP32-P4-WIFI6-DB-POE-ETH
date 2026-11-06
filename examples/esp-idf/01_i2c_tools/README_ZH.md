# I2C 工具

[English](README.md) | [简体中文](README_ZH.md)

## 功能

提供串口控制台，通过 `i2cconfig`、`i2cdetect`、`i2cget`、`i2cset` 和 `i2cdump` 扫描和读取 I2C 设备。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- USB 数据线

## 怎么运行

```bash
idf.py set-target esp32p4 build flash monitor
```

在控制台先配置板载总线，再使用其他命令：

```text
i2cconfig --scl=8 --sda=7
i2cdetect
```

## 预期现象

出现 `i2c-tools>` 提示符，`i2cdetect` 打印有响应的地址表。

## 常见问题

必须先运行 `i2cconfig`，其他命令依赖它。
