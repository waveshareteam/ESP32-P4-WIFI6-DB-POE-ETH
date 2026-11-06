# 固件

[English](README.md) | [简体中文](README_ZH.md)

出厂固件已在 ESP32-P4-WIFI6-DB-POE-ETH 搭配 10.1 英寸屏幕的配置上实测。该合并镜像从 `0x0` 地址烧录。

| 文件 | 用途 |
| --- | --- |
| `ESP32-P4-WIFI6-DB-POE-ETH_FactoryOnly_20261027.bin` | 恢复 10.1 英寸屏幕的出厂演示 |
| `ESP32-P4-WIFI6-DB-POE-ETH_ESP32-C5-WIFI-STA_20261027.bin` | ESP32-C5 Wi-Fi 协处理器固件 |

擦除 Flash 会清除已有数据。请将 `PORT` 替换为开发板的串口。

```bash
python -m pip install esptool
esptool --chip esp32p4 --port PORT erase-flash
esptool --chip esp32p4 --port PORT write-flash 0x0 ESP32-P4-WIFI6-DB-POE-ETH_FactoryOnly_20261027.bin
```

烧录 ESP32-C5 镜像时，请连接协处理器的下载口，并将 `C5_PORT` 替换为该串口：

```bash
esptool --chip esp32c5 --port C5_PORT write-flash 0x0 ESP32-P4-WIFI6-DB-POE-ETH_ESP32-C5-WIFI-STA_20261027.bin
```

[出厂固件源码](../examples/factory/)位于本仓库。
