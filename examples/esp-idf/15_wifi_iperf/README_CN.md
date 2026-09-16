# Wi-Fi iperf

本示例用于测试 ESP32-P4-WIFI6-DB-POE-ETH 开发板的 Wi-Fi 吞吐。
ESP32-P4 是 ESP-Hosted Host，板载 ESP32-C5 是 Wi-Fi 6 协处理器，二者通过板载
4-bit SDIO 链路通信。P4 串口控制台负责让 C5 连接 AP，并运行 iperf 客户端或服务端。

```text
examples/esp-idf/15_wifi_iperf/
├── cp/            ESP32-C5 协处理器固件
└── mcu_host/      ESP32-P4 Host 控制台应用
```

## 前提条件

- 已进入提供 `idf.py` 的 ESP-IDF 环境。
- 可访问 ESP32-C5 与 ESP32-P4 对应的串口。
- PC 与开发板连接到同一局域网。为了获得更有代表性的结果，建议 PC 通过网线接入 AP，
  而非与开发板共用同一个 Wi-Fi 空口。
- PC 请使用 **iperf 2.x**。固件组件实现的是 iperf 2 协议，不能与 `iperf3` 互通。

## 1. 烧录 ESP32-C5 协处理器

在仓库根目录下执行：

```bash
cd examples/esp-idf/15_wifi_iperf/cp
idf.py set-target esp32c5
idf.py menuconfig
idf.py -p COM<cp-port> flash monitor
```

默认配置已启用 Wi-Fi 功能并使用 SDIO。请保持协处理器 target 为 `esp32c5`，并使其
传输配置与 Host 保持一致。

## 2. 烧录 ESP32-P4 Host

打开另一个 ESP-IDF 终端并执行：

```bash
cd examples/esp-idf/15_wifi_iperf/mcu_host
idf.py set-target esp32p4
idf.py menuconfig
idf.py -p COM<host-port> flash monitor
```

Host 默认配置选择板载 ESP32-C5 与 4-bit SDIO，同时保留 ESP-Hosted RPC/Wi-Fi 依赖，
以及面向吞吐测试的 Wi-Fi 和 LWIP 缓冲区设置。

## 3. 在 P4 控制台连接 AP

P4 启动后的提示符为 `iperf>`。`wifi_cmd` 是组件名称，不是控制台命令；请使用实际注册的
station 命令：

```text
iperf> sta_scan
iperf> sta_connect <SSID> <password>
```

请等待关联事件和 IPv4 事件都出现，例如：

```text
WIFI_EVENT_STA_CONNECTED
IP_EVENT_STA_GOT_IP4:[...][ip=192.168.x.x][mask=...][gw=...]
```

只有看到 `IP_EVENT_STA_GOT_IP4` 后才能开始 iperf。该日志中的 P4 地址，是 PC 作为
iperf 客户端时所需的目标地址。

常用命令：

```text
iperf> sta_disconnect
iperf> ping -c 5 <PC_IP>
iperf> iperf --help
```

> [!WARNING]
> 当前依赖版本中，成功关联后输入 `wifi status` 可能使 P4 重启，原因是 station-AID RPC
> 未被打包。请使用上面的 `IP_EVENT_STA_GOT_IP4` 日志确认 Host 地址。

## 4. 测试 TCP 吞吐

一端运行服务端，另一端运行客户端。PC 请勿使用 `iperf3.exe`，请使用 iperf 2 的
`iperf.exe`。

### P4/C5 到 PC

在 PC 上启动并保持 iperf 服务端运行：

```powershell
iperf.exe -s -p 5001
```

在 P4 控制台执行，将 `<PC_IP>` 替换为 PC 的 IPv4 地址：

```text
iperf> iperf -c <PC_IP> -p 5001 -t 30 -i 1
```

控制台会立即回到 `iperf>`，这是流量任务在后台运行的正常表现。成功时会输出
`Successfully connected`、每秒带宽以及 30 秒汇总结果。

### PC 到 P4/C5

在 P4 控制台启动服务端，并保持其运行：

```text
iperf> iperf -s -p 5001
```

在 PC 上执行，将 `<P4_IP>` 替换为 `IP_EVENT_STA_GOT_IP4` 中的地址：

```powershell
iperf.exe -c <P4_IP> -p 5001 -t 30 -i 1
```

两个方向应分开测试。对比两端最终 30 秒平均值；两个报告存在小幅差异属于正常现象。

### UDP 示例

先在 PC 上启动 UDP 服务端：

```powershell
iperf.exe -s -u -p 5001
```

然后从 P4 发送 20 Mbit/s 的 UDP 流量：

```text
iperf> iperf -c <PC_IP> -u -b 20 -p 5001 -t 30 -i 1
```

使用 `iperf --abort` 可以停止正在运行的测试。
