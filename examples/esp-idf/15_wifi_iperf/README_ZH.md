# Wi-Fi iPerf

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 ESP-Hosted SDIO 链路测量 Wi-Fi 吞吐，由 ESP32-C5 提供射频，对端是运行 iPerf 2.x 的电脑。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 与电脑在同一局域网的 Wi-Fi 接入点
- 安装 iPerf 2.x 的电脑

## 怎么运行

先从 [`cp`](cp/README_ZH.md) 烧录 ESP32-C5 协处理器，再从本目录烧录 ESP32-P4 主机：

```bash
cd cp
idf.py set-target esp32c5 build
idf.py -p <C5_PORT> flash
cd ..
idf.py set-target esp32p4 menuconfig
idf.py build
idf.py -p <P4_PORT> flash monitor
```

在电脑上运行 `iperf.exe -s -p 5001`，然后在 `iperf>` 提示符下运行：

```text
sta_connect <SSID> <password>
iperf -c <PC_IP> -p 5001 -t 30 -i 1
```

## 预期现象

出现 `IP_EVENT_STA_GOT_IP4` 后，iperf 打印每秒带宽和 30 秒汇总。

## 常见问题

不兼容 iPerf3。连接成功后运行 `wifi status` 可能导致 ESP32-P4 复位，请通过 `IP_EVENT_STA_GOT_IP4` 日志查看地址。
