# 固件

[English](README.md) | [简体中文](README_ZH.md)

出厂固件为合并镜像，从 `0x0` 地址烧录，请按所使用的屏幕尺寸选择对应文件。

| 文件 | 用途 |
| --- | --- |
| [ESP32-P4-WIFI6-DB-POE-ETH-10.1-DSI-TOUCH-A_Factory_20261117.bin](ESP32-P4-WIFI6-DB-POE-ETH-10.1-DSI-TOUCH-A_Factory_20261117.bin) | 适配 [10.1-DSI-TOUCH-A](https://www.waveshare.net/shop/10.1-DSI-TOUCH-A.htm) 屏幕的出厂演示固件 |
| [ESP32-P4-WIFI6-DB-POE-ETH-5-DSI-TOUCH-A_Factory_20261117.bin](ESP32-P4-WIFI6-DB-POE-ETH-5-DSI-TOUCH-A_Factory_20261117.bin) | 适配 [5-DSI-TOUCH-A](https://www.waveshare.net/shop/5-DSI-TOUCH-A.htm) 屏幕的出厂演示固件 |
| [ESP32-P4-WIFI6-DB-POE-ETH-8-DSI-TOUCH-A_Factory_20261117.bin](ESP32-P4-WIFI6-DB-POE-ETH-8-DSI-TOUCH-A_Factory_20261117.bin) | 适配 [8-DSI-TOUCH-A](https://www.waveshare.net/shop/8-DSI-TOUCH-A.htm) 屏幕的出厂演示固件 |
| [ESP32-P4-WIFI6-DB-POE-ETH_ESP32-C5-WIFI-STA_20261112.bin](ESP32-C5/ESP32-P4-WIFI6-DB-POE-ETH_ESP32-C5-WIFI-STA_20261112.bin) | ESP32-C5 Wi-Fi 协处理器固件 |

擦除 Flash 会清除已有数据。以下以 10.1-DSI-TOUCH-A 为例，将 `PORT` 替换为开发板的串口，其他屏幕使用表中对应的文件名。

```bash
python -m pip install esptool
esptool --chip esp32p4 --port PORT erase-flash
esptool --chip esp32p4 --port PORT write-flash 0x0 ESP32-P4-WIFI6-DB-POE-ETH-10.1-DSI-TOUCH-A_Factory_20261117.bin
```

烧录 ESP32-C5 镜像时，请连接协处理器的下载口，并将 `C5_PORT` 替换为该串口：

```bash
esptool --chip esp32c5 --port C5_PORT write-flash 0x0 ESP32-C5/ESP32-P4-WIFI6-DB-POE-ETH_ESP32-C5-WIFI-STA_20261112.bin
```

[出厂固件源码](../examples/factory/)位于本仓库。
