# 以太网基础

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 `esp_netif` 启动 RMII 以太网，并使用 DHCP 获取地址。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 网线和提供 DHCP 的网络

## 怎么运行

```bash
idf.py set-target esp32p4 build flash monitor
```

## 预期现象

日志显示 `Ethernet Link Up`，随后显示 `Ethernet Got IP Address` 和地址，可从电脑 ping 通。

## 常见问题

没有获取到地址时，检查网线和 DHCP 服务。
