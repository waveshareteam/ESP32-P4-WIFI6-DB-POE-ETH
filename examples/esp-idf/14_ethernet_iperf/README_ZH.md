# 以太网 iPerf

[English](README.md) | [简体中文](README_ZH.md)

## 功能

在 `iperf>` 控制台中测量以太网 TCP 和 UDP 吞吐，对端是运行 iPerf 2.x 的电脑。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 网线和提供 DHCP 的网络
- 安装 iPerf 2.x 的电脑

## 怎么运行

```bash
idf.py set-target esp32p4 build flash monitor
```

在电脑上运行 `iperf -u -s -i 3`，然后在 `iperf>` 提示符下运行：

```text
iperf -u -c PC_IP -i 3 -t 30
```

## 预期现象

日志显示 `eth0 ip:` 和 `iperf>` 提示符，电脑端报告带宽、抖动和丢包数。

## 常见问题

DHCP 成功后控制台才会启动。不兼容 iPerf3，电脑防火墙需要放行 5001 端口。
