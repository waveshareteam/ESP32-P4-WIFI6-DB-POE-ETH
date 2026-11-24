# eth_ip101

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过板载 IP101 PHY 演示以太网事件，并在获取 IP 地址后发送 HTTP 请求。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- USB 数据线和网线
- 已启用 DHCP 的路由器或网络

## 怎么运行

1. 安装[仓库首页](../../../../README_ZH.md)列出的 Arduino-ESP32 版本，并按[板卡定义安装说明](../../libraries/board_config/README.md)添加开发板。
2. 接上网线，在 Arduino IDE 中选择 `Waveshare ESP32-P4-WIFI6-DB-POE-ETH`，上传 `eth_ip101.ino`。
3. 以 115200 波特率打开串口监视器。

## 预期现象

链路建立时，串口显示 `ETH Connected`。DHCP 分配地址后显示 `ETH Got IP`，随后示例向 `google.com:80` 发送 HTTP 请求，并打印收到的响应。

## 常见问题

如果显示 `ETH Connected` 后一直没有出现 `ETH Got IP`，请检查网络的 DHCP 设置。如果已获取 IP 地址但显示 `connection failed`，请检查 DNS 和外网连接。
