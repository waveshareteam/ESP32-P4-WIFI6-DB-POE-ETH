# I2C 工具

[English](README.md) | [简体中文](README_ZH.md)

## 功能

提供串口控制台，通过 `i2cconfig`、`i2cdetect`、`i2cget`、`i2cset` 和 `i2cdump` 扫描和读取 I2C 设备。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- USB 数据线

## 怎么运行

```bash
idf.py set-target esp32p4
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth
idf.py build flash monitor
```

程序启动时会自动初始化板载 I2C：SDA 使用 GPIO7，SCL 使用 GPIO8，频率为 100 kHz。需要时仍可使用 `i2cconfig` 修改配置：

```bash
i2cdetect

i2cconfig --scl=8 --sda=7 --freq=100000
i2cdetect
```

## 预期现象

出现 `i2c-tools>` 提示符，`i2cdetect` 打印有响应的地址表。

## 常见问题

默认总线使用 SDA GPIO7、SCL GPIO8。只有需要修改引脚或频率时才需要运行 `i2cconfig`。
