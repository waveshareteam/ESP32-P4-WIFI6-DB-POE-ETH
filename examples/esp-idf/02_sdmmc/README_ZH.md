# SD 卡（SDMMC）

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 4 线 SDMMC 挂载 microSD 卡，打印卡信息，然后写入、重命名并读回测试文件。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- microSD 卡

## 怎么运行

```bash
idf.py set-target esp32p4 build flash monitor
```

## 预期现象

串口监视器显示卡的名称、类型、容量和速度，然后显示从测试文件读回的内容。

## 常见问题

请先备份存储卡：在 `SD/MMC Example Configuration` 中启用格式化选项会清除卡上的数据。
