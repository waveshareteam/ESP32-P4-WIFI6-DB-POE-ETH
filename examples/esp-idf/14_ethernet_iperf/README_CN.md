# 以太网 iPerf 示例

[English Version](./README.md)

本示例用于测量 ESP32-P4-WIFI6-DB-POE-ETH 开发板的以太网 TCP/UDP 吞吐。
`07_ethernet_basic` 只验证以太网初始化、DHCP 和基础 IP 连通性；本示例在开发板通过
DHCP 获取 IPv4 地址后，提供交互式 `iperf>` 命令行用于吞吐测试。

## 本板以太网配置

示例使用 ESP32-P4 内部 EMAC，经 RMII 连接板载 IP101 PHY：

| 信号 | GPIO / 配置 |
| --- | --- |
| PHY | IP101，地址 1 |
| MDC / MDIO | GPIO31 / GPIO52 |
| PHY 复位 | GPIO51 |
| RMII REF_CLK | GPIO50，外部输入 50 MHz |
| TX_EN / TXD0 / TXD1 | GPIO49 / GPIO34 / GPIO35 |
| CRS_DV / RXD0 / RXD1 | GPIO28 / GPIO29 / GPIO30 |

将开发板和测试 PC 接到同一个以太网网络，且该网络必须提供 DHCP。本示例在拿到 IPv4
地址之前会等待，因此不会显示 `iperf>` 提示符。

开发板内置命令与 iPerf 2.x 兼容，不兼容 iPerf3。

## 构建与运行

在本示例目录执行：

```powershell
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

将 `PORT` 替换为开发板串口。链路协商和 DHCP 成功后，日志会出现 `eth0 ip: ...`，随后显示
`iperf>`。输入以下命令可查看开发板 IP：

```text
ethernet info
```

若没有出现提示符，表示开发板尚未取得 DHCP 地址。应先检查网线、交换机/路由器端口与 DHCP
服务，再开始吞吐测试。

## Windows PC 准备

从 [iPerf2 下载页](https://sourceforge.net/projects/iperf2/files/) 获取 Windows 版 iPerf 2.x。
以下命令统一使用 `iperf`；若实际文件名不同，请自行调整可执行文件名或将其目录加入 `PATH`。
其中的 IP 地址请替换为 PC 有线网卡 IPv4 地址和开发板实际 IP。

Windows 防火墙需要允许该测试程序接收 UDP/TCP 5001 端口的数据。

## UDP 吞吐测试

### 上行：ESP32-P4 -> PC

PC 开启服务端：

```powershell
iperf -u -s -i 3
```

`-s` 表示持续监听，按 `Ctrl+C` 才会停止，并不是扫描局域网。然后在开发板输入，将 `PC_IP`
替换为 PC 有线网卡的 IPv4 地址：

```text
iperf -u -c PC_IP -i 3 -t 30
```

### 下行：PC -> ESP32-P4

开发板开启服务端：

```text
iperf -u -s -t 30 -i 3
```

PC 执行：

```powershell
iperf -u -c ESP_IP -b 80M -t 30 -i 3
```

先以 `80M` 测试；如果丢包可接受，再依次尝试 `85M`、`90M`、`95M`。

## 如何判断结果

- 100 Mbps 是 PHY 链路速率；UDP 应用层吞吐会受到以太网、IP、UDP 和软件协议栈开销影响，
  因而低于 100 Mbps 属于正常现象。
- UDP 需同时记录带宽、抖动和 PC 端 `Lost/Total Datagrams`。出现持续丢包前可达到的最高速率，
  比只看一次设定速率更有参考价值。
- PC 的 UDP 服务端在空闲时会输出 `0.000 Bytes`；这只表示该统计周期没有收到测试数据，
  不表示正在扫描网络，也不表示链路速率为 0 Mbps。
- 对比不同测试结果时，应保持开发板、PC、网线和交换机端口均协商在 100 Mbps 全双工。
