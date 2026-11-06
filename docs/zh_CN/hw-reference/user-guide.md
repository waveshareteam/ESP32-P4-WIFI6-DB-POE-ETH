# ESP32-P4-WIFI6-DB-POE-ETH <!-- omit in toc -->

[English](../../en/hw-reference/user-guide.md)

本指南将帮助你快速上手 ESP32-P4-WIFI6-DB-POE-ETH，并提供更深入的硬件信息。

ESP32-P4-WIFI6-DB-POE-ETH 是一款以 ESP32-P4NRW32X 为主处理器、通过 SDIO 搭配 ESP32-C5 无线协处理器的开发板。板载集成 PoE 模块的 RMII 以太网、双 lane MIPI-DSI 显示与触摸接口、双 lane MIPI-CSI 摄像头接口、ES8311 音频、microSD、USB Host 和 40 针 GPIO 排针。

本文档包括以下主要内容：

- [入门指南](#1-入门指南)：开发板概述、板载资源、外设和出厂固件。
- [开始开发应用](#2-开始开发应用)：开发前的硬件和软件准备。
- [硬件参考](#3-硬件参考)：GPIO 分配、GPIO 排针、接口和使用注意事项。
- [相关文档](#4-相关文档)：相关文档链接。

# 1. 入门指南

## 1.1. 概述

ESP32-P4NRW32X 主处理器负责显示、摄像头、音频、存储、以太网和 USB 接口。ESP32-C5 作为无线协处理器，通过 ESP-Hosted（SDIO）为 ESP32-P4 提供 Wi-Fi 6 和 Bluetooth LE。ESP32-P4 本身不集成 Wi-Fi 和蓝牙，因此无线功能需要 P4 应用与 C5 协处理器固件配合运行。

## 1.2. 板载资源

下表列出开发板的主要组件。

<details open>
<summary><b>板载资源表</b></summary>

| 主要组件 | 说明 |
| --- | --- |
| 主处理器 | ESP32-P4NRW32X，双核高性能 RISC-V + 低功耗核，封装内集成 PSRAM，工作频率 200 MHz。 |
| Flash | 32 MB，QIO 模式。 |
| 无线协处理器 | ESP32-C5，支持 Wi-Fi 6 和 Bluetooth LE，通过 4-bit SDIO 与 ESP32-P4 连接。ESP32-P4 GPIO54 复位 ESP32-C5（低电平有效），GPIO6 可用作其唤醒信号。 |
| 以太网 | RMII MAC + IP101 PHY，PHY 地址为 1，50 MHz REF_CLK 外部输入。板载 PoE 模块，可由支持 IEEE 802.3af 的 PoE 路由器或交换机供电。 |
| 显示 | 双 lane MIPI-DSI，外接微雪屏幕（见[显示与摄像头接口](#33-显示与摄像头接口)）。LCD 复位引脚未接，背光通过 I2C 控制。 |
| 触摸 | GT911 电容触摸，接在 GPIO7/GPIO8 共享 I2C 总线上；复位和中断未接，触摸状态通过轮询读取。 |
| 音频 | ES8311 编解码器，I2C + I2S，MCLK 接 GPIO13；扬声器经功放输出，功放由 GPIO53 使能；单路模拟麦克风输入。 |
| 存储 | microSD 卡槽，4-bit SDMMC；卡电源由 GPIO45 控制（低电平有效）。 |
| 摄像头 | 双 lane MIPI-CSI 接口，SCCB 接共享 I2C 总线，支持 OV5647（[RPi Camera (B)](https://www.waveshare.net/shop/RPi-Camera-B.htm)）和 SC2336。XCLK 和 RESET 未接 ESP32-P4 GPIO。 |
| USB | USB 2.0 Host，接 ESP32-P4 内置 USB PHY（DM/DP），带过流保护。 |
| UART0 | 固件下载和串口日志，GPIO37（TX）/ GPIO38（RX）。 |
| GPIO 排针 | 40 针排针 P6（见 [GPIO 排针（P6）](#32-gpio-排针p6)），引出 28 个 ESP32-P4 GPIO 以及 3.3 V、5 V 和 GND；部分 GPIO 与板载功能共用。 |
| 32.768 kHz 晶振 | 接 GPIO0/GPIO1（XTAL_32K_N/P），为 ESP32-P4 提供 RTC 慢时钟。 |

</details>

## 1.3. 外设速查

<details open>
<summary><b>外设速查表</b></summary>

| 模块 | 器件 / 功能 | 接口 | 地址 / 参数 | GPIO / 信号 |
| --- | --- | --- | --- | --- |
| LCD | MIPI-DSI 显示屏 | MIPI-DSI | 2-lane；HX8394 / ILI9881C / JD9365 | LCD 复位未接 |
| LCD 背光 | 背光控制器 | I2C | 7-bit 地址 0x45 | SDA=GPIO7，SCL=GPIO8 |
| 触摸 | GT911 电容触摸 | I2C | 7-bit 地址 0x5D 或 0x14 | SDA=GPIO7，SCL=GPIO8；RST/INT 未接 |
| 摄像头 | MIPI-CSI 摄像头接口 | MIPI-CSI + SCCB | 2-lane；OV5647 / SC2336 | SCCB 复用 GPIO7/8；XCLK/RESET 未接 |
| 音频 | ES8311 编解码器 | I2C + I2S | 7-bit 地址 0x18 | SDA=7，SCL=8；MCLK=13，SCLK=12，WS=10，DOUT=9，DIN=11 |
| 功放 | 扬声器功放使能 | GPIO | 高电平有效 | GPIO53 |
| microSD | microSD 卡槽 | SDMMC 4-bit | — | CLK=43，CMD=44，D0–D3=39–42；电源使能=45，低电平有效 |
| 以太网 | IP101 PHY | RMII + SMI | PHY 地址 1；50 MHz REF_CLK 输入 | CRS_DV=28，RXD0=29，RXD1=30，TXD0=34，TXD1=35，TX_EN=49，REF_CLK=50；MDC=31，MDIO=52；PHY 复位=51 |
| Wi-Fi/BT 协处理器 | ESP32-C5 | SDIO 4-bit | 40 MHz | CLK=18，CMD=19，D0–D3=14–17；复位=54（低电平有效）；唤醒=6 |
| USB | USB 2.0 Host | USB | 内置 USB PHY，带过流保护 | 专用 DM/DP 引脚 |
| UART0 | 烧录与串口日志 | UART | — | TX=GPIO37，RX=GPIO38 |

</details>

## 1.4. 出厂固件

开发板的应用固件基于 ESP-Brookesia，包含显示、触摸、音频、摄像头、视频、Wi-Fi、以太网和小智等功能。功能介绍、使用方法和编译说明见 [Brookesia 固件 README](../../../firmware/brookesia/README_CN.md)。通过串口控制台测试整板硬件的固件见[整板测试固件 README](../../../firmware/factory_test/README_CN.md)。

# 2. 开始开发应用

本节介绍开发所需的硬件，以及开发前的硬件和软件准备。

## 2.1. 必备硬件

- 1 x ESP32-P4-WIFI6-DB-POE-ETH
- 1 x USB 线，用于供电、固件下载和串口日志
- 1 x 电脑（Windows、Linux 或 macOS）

## 2.2. 可选硬件

- 1 x 微雪 MIPI-DSI 触摸屏：[5-DSI-TOUCH-A](https://www.waveshare.net/shop/5-DSI-TOUCH-A.htm)、[7-DSI-TOUCH-A](https://www.waveshare.net/shop/7-DSI-TOUCH-A.htm)、[8-DSI-TOUCH-A](https://www.waveshare.net/shop/8-DSI-TOUCH-A.htm) 或 [10.1-DSI-TOUCH-A](https://www.waveshare.net/shop/10.1-DSI-TOUCH-A.htm)
- 1 x MIPI-CSI 摄像头模组（OV5647，如 [RPi Camera (B)](https://www.waveshare.net/shop/RPi-Camera-B.htm)，或 SC2336）
- 1 x microSD 卡
- 1 x 扬声器
- 1 x 网线，PoE 供电时还需要支持 IEEE 802.3af 的 PoE 路由器或交换机

## 2.3. 供电方式

开发板可通过以下任一方式供电：

- USB 接口
- 以太网口 PoE 供电，需连接支持 IEEE 802.3af 的路由器或交换机

> **注意**
>
> 固件无法检测开发板是否由 PoE 供电。验证 PoE 时，请在仅用 PoE 供电的情况下重新测试一遍。

## 2.4. 硬件准备

1. 用 USB 线将开发板的下载/串口端口连接到电脑。
2. 如使用屏幕或摄像头，请在断电状态下连接排线，并对照模组说明确认接口方向。
3. 如使用以太网，请连接到有 DHCP 服务器的网络；PoE 供电时请使用 IEEE 802.3af 端口。
4. 如自动下载失败，请在烧录前手动让开发板进入下载模式。

## 2.5. 软件准备

1. 安装 ESP-IDF。本地 BSP 要求 ESP-IDF >= 5.5。
2. 编译并烧录板级检查示例 [00_board_check](../../../examples/esp-idf/00_board_check/)：

   ```bash
   cd examples/esp-idf/00_board_check
   idf.py set-target esp32p4
   idf.py build
   idf.py -p PORT flash monitor
   ```

   将 `PORT` 替换为实际串口，例如 Windows 下的 `COM7` 或 Linux 下的 `/dev/ttyACM0`。

3. 板级检查通过后，可继续运行 [ESP-IDF 示例](../../../examples/esp-idf/)或 [Arduino 示例](../../../examples/Arduino/)。

# 3. 硬件参考

本节提供开发板硬件的详细信息。

## 3.1. GPIO 分配

下表列出全部 GPIO（0–54）及其板载连接。

<details open>
<summary><b>GPIO 分配表（GPIO0–GPIO54）</b></summary>

| GPIO | 信号 | 连接到 | 备注 |
| --- | --- | --- | --- |
| GPIO0 | XTAL_32K_N | 32.768 kHz 晶振 / 排针 P6-23 | 用作其他用途前需处理晶振电路 |
| GPIO1 | XTAL_32K_P | 32.768 kHz 晶振 / 排针 P6-22 | 用作其他用途前需处理晶振电路 |
| GPIO2 | GPIO2 | 排针 P6-21 | 可配置为普通 GPIO |
| GPIO3 | GPIO3 | 排针 P6-19 | 可配置为普通 GPIO |
| GPIO4 | GPIO4 | 排针 P6-18 | 可配置为普通 GPIO |
| GPIO5 | GPIO5 | 排针 P6-16 | 可配置为普通 GPIO |
| GPIO6 | C5_WAKEUP | ESP32-C5 / 排针 P6-15 | 可用作 ESP32-C5 唤醒信号 |
| GPIO7 | I2C SDA | ES8311、GT911、LCD 背光控制器、摄像头 SCCB / 排针 P6-3 | 共享 I2C 总线 |
| GPIO8 | I2C SCL | ES8311、GT911、LCD 背光控制器、摄像头 SCCB / 排针 P6-5 | 共享 I2C 总线 |
| GPIO9 | I2S DOUT | ES8311 DSDIN | 播放数据，ESP32-P4 → codec |
| GPIO10 | I2S WS | ES8311 LRCK | 音频帧同步 |
| GPIO11 | I2S DIN | ES8311 ASDOUT | 采集数据，codec → ESP32-P4 |
| GPIO12 | I2S SCLK | ES8311 SCLK | 音频位时钟 |
| GPIO13 | I2S MCLK | ES8311 MCLK | 音频主时钟 |
| GPIO14 | SDIO D0 | ESP32-C5 | SDIO 数据线 |
| GPIO15 | SDIO D1 | ESP32-C5 | SDIO 数据线 |
| GPIO16 | SDIO D2 | ESP32-C5 | SDIO 数据线 |
| GPIO17 | SDIO D3 | ESP32-C5 | SDIO 数据线 |
| GPIO18 | SDIO CLK | ESP32-C5 | SDIO 时钟 |
| GPIO19 | SDIO CMD | ESP32-C5 | SDIO 命令 |
| GPIO20 | GPIO20 | 排针 P6-13 | 可配置为普通 GPIO |
| GPIO21 | GPIO21 | 排针 P6-11 | 可配置为普通 GPIO |
| GPIO22 | GPIO22 | 排针 P6-12 | 可配置为普通 GPIO |
| GPIO23 | GPIO23 | 排针 P6-7 | 可配置为普通 GPIO |
| GPIO24 | GPIO24 | 排针 P6-27 | 可配置为普通 GPIO |
| GPIO25 | GPIO25 | 排针 P6-28 | 可配置为普通 GPIO |
| GPIO26 | GPIO26 | 排针 P6-31 | 可配置为普通 GPIO |
| GPIO27 | GPIO27 | 排针 P6-38 | 可配置为普通 GPIO |
| GPIO28 | RMII CRS_DV | 以太网 PHY | 载波侦听 / 接收数据有效，PHY → ESP32-P4 |
| GPIO29 | RMII RXD0 | 以太网 PHY | 接收数据位 0，PHY → ESP32-P4 |
| GPIO30 | RMII RXD1 | 以太网 PHY | 接收数据位 1，PHY → ESP32-P4 |
| GPIO31 | SMI MDC | 以太网 PHY | PHY 管理时钟，ESP32-P4 → PHY |
| GPIO32 | GPIO32 | 排针 P6-26 | 可配置为普通 GPIO |
| GPIO33 | GPIO33 | 排针 P6-29 | 可配置为普通 GPIO |
| GPIO34 | RMII TXD0 | 以太网 PHY | 发送数据位 0，ESP32-P4 → PHY；Strapping 引脚 |
| GPIO35 | RMII TXD1 | 以太网 PHY | 发送数据位 1，ESP32-P4 → PHY；Strapping 引脚 |
| GPIO36 | GPIO36 | 排针 P6-24 | 可配置为普通 GPIO；Strapping 引脚 |
| GPIO37 | UART0 TX | 下载/串口 / 排针 P6-8 | 固件下载和串口日志；Strapping 引脚 |
| GPIO38 | UART0 RX | 下载/串口 / 排针 P6-10 | 固件下载和串口日志；Strapping 引脚 |
| GPIO39 | SD D0 | microSD 卡 | SDMMC 数据线 |
| GPIO40 | SD D1 | microSD 卡 | SDMMC 数据线 |
| GPIO41 | SD D2 | microSD 卡 | SDMMC 数据线 |
| GPIO42 | SD D3 | microSD 卡 | SDMMC 数据线 |
| GPIO43 | SD CLK | microSD 卡 | SDMMC 时钟 |
| GPIO44 | SD CMD | microSD 卡 | SDMMC 命令 |
| GPIO45 | SD_VDD_EN | microSD 电源开关 / 排针 P6-40 | 低电平有效；去掉 R30 可释放（见 [microSD 电源控制（R30）](#34-microsd-电源控制r30)） |
| GPIO46 | GPIO46 | 排针 P6-36 | 可配置为普通 GPIO |
| GPIO47 | GPIO47 | 排针 P6-37 | 可配置为普通 GPIO |
| GPIO48 | GPIO48 | 排针 P6-33 | 可配置为普通 GPIO |
| GPIO49 | RMII TX_EN | 以太网 PHY | 发送使能，ESP32-P4 → PHY |
| GPIO50 | RMII REF_CLK | 以太网 PHY | 50 MHz 参考时钟，PHY → ESP32-P4 |
| GPIO51 | PHY_RST | 以太网 PHY | PHY 硬件复位，ESP32-P4 → PHY |
| GPIO52 | SMI MDIO | 以太网 PHY | PHY 管理数据，双向 |
| GPIO53 | PA_CTRL | 扬声器功放 / 排针 P6-35 | 高电平有效 |
| GPIO54 | C5_RST | ESP32-C5 复位 / 排针 P6-32 | 低电平复位，高电平使能 |

</details>

## 3.2. GPIO 排针（P6）

P6 是 40 针排针，引出 28 个 ESP32-P4 GPIO 以及 3.3 V、5 V 和 GND。5 V 引脚由 SMF5.0CA TVS 二极管 D1 保护。标 \* 的引脚与板载功能共用，外接电路前请参阅[使用注意事项](#35-使用注意事项)。

<details open>
<summary><b>P6 排针（40 针）</b></summary>

<table>
  <tr>
    <td valign="middle"><img src="../../../assert/P6.webp" alt="P6 排针" width="350"></td>
    <td valign="middle">
      <table>
        <tr><th>信号</th><th>引脚</th><th>引脚</th><th>信号</th></tr>
        <tr><td>3V3</td><td align="center">1</td><td align="center">2</td><td>5V</td></tr>
        <tr><td>GPIO7（I2C SDA）*</td><td align="center">3</td><td align="center">4</td><td>5V</td></tr>
        <tr><td>GPIO8（I2C SCL）*</td><td align="center">5</td><td align="center">6</td><td>GND</td></tr>
        <tr><td>GPIO23</td><td align="center">7</td><td align="center">8</td><td>GPIO37（UART0 TX）*</td></tr>
        <tr><td>GND</td><td align="center">9</td><td align="center">10</td><td>GPIO38（UART0 RX）*</td></tr>
        <tr><td>GPIO21</td><td align="center">11</td><td align="center">12</td><td>GPIO22</td></tr>
        <tr><td>GPIO20</td><td align="center">13</td><td align="center">14</td><td>GND</td></tr>
        <tr><td>GPIO6（C5 唤醒）*</td><td align="center">15</td><td align="center">16</td><td>GPIO5</td></tr>
        <tr><td>3V3</td><td align="center">17</td><td align="center">18</td><td>GPIO4</td></tr>
        <tr><td>GPIO3</td><td align="center">19</td><td align="center">20</td><td>GND</td></tr>
        <tr><td>GPIO2</td><td align="center">21</td><td align="center">22</td><td>GPIO1（XTAL_32K_P）*</td></tr>
        <tr><td>GPIO0（XTAL_32K_N）*</td><td align="center">23</td><td align="center">24</td><td>GPIO36</td></tr>
        <tr><td>GND</td><td align="center">25</td><td align="center">26</td><td>GPIO32</td></tr>
        <tr><td>GPIO24</td><td align="center">27</td><td align="center">28</td><td>GPIO25</td></tr>
        <tr><td>GPIO33</td><td align="center">29</td><td align="center">30</td><td>GND</td></tr>
        <tr><td>GPIO26</td><td align="center">31</td><td align="center">32</td><td>GPIO54（C5 复位）*</td></tr>
        <tr><td>GPIO48</td><td align="center">33</td><td align="center">34</td><td>GND</td></tr>
        <tr><td>GPIO53（功放使能）*</td><td align="center">35</td><td align="center">36</td><td>GPIO46</td></tr>
        <tr><td>GPIO47</td><td align="center">37</td><td align="center">38</td><td>GPIO27</td></tr>
        <tr><td>GND</td><td align="center">39</td><td align="center">40</td><td>GPIO45（SD 电源）*</td></tr>
      </table>
    </td>
  </tr>
</table>

</details>

- 空闲 GPIO：GPIO2–5、GPIO20–27、GPIO32、GPIO33、GPIO36、GPIO46–48。
- GPIO7/GPIO8 是板载 I2C 总线，可在此外接 I2C 设备，但需避免地址冲突。
- GPIO37/GPIO38 是用于烧录和日志的 UART0。

## 3.3. 显示与摄像头接口

摄像头 SCCB、GT911 触摸控制器、LCD 背光控制器和 ES8311 共用 GPIO7/GPIO8 上的 I2C。

| 接口 | 类型 | ESP32-P4 信号 |
| --- | --- | --- |
| 显示 | MIPI-DSI | 两条数据 lane、时钟 lane，触摸和背光使用共享 I2C；LCD 复位、触摸复位和触摸中断未接 |
| 摄像头 | MIPI-CSI | 两条数据 lane、时钟 lane，SCCB 使用共享 I2C；XCLK/RESET 未接 |

支持的微雪屏幕如下。在 `menuconfig` 中通过 BSP 的 LCD 类型选项（`CONFIG_BSP_LCD_TYPE_*`）选择实际连接的屏幕，默认是 10.1 英寸。

| 屏幕 | 控制器 | 分辨率 |
| --- | --- | --- |
| [5-DSI-TOUCH-A](https://www.waveshare.net/shop/5-DSI-TOUCH-A.htm) | HX8394，5 英寸 | 720 × 1280 |
| [7-DSI-TOUCH-A](https://www.waveshare.net/shop/7-DSI-TOUCH-A.htm) | ILI9881C，7 英寸 | 720 × 1280 |
| [8-DSI-TOUCH-A](https://www.waveshare.net/shop/8-DSI-TOUCH-A.htm) | JD9365，8 英寸 | 800 × 1280 |
| [10.1-DSI-TOUCH-A](https://www.waveshare.net/shop/10.1-DSI-TOUCH-A.htm) | JD9365，10.1 英寸 | 800 × 1280 |

## 3.4. microSD 电源控制（R30）

GPIO45 由排针 P6-40 和 microSD 电源开关共用。它经 R30（0 Ω）驱动 P-MOSFET Q1（AO3401）的栅极，R27（10 kΩ）将栅极下拉到地，因此 GPIO45 为低电平或悬空时卡供电，GPIO45 为高电平时断电。如无需用软件控制 SD 卡电源，可去掉 R30，将 GPIO45 释放出来。

<center>

| ![microSD 电源控制电路](../../../assert/GPIO45.webp) |
|:--:|
| microSD 电源控制电路 |

</center>

| 配置 | R30 | microSD 电源 | GPIO45 |
| --- | --- | --- | --- |
| 默认 | 0 Ω | 由 GPIO45 控制：低电平供电，高电平断电 | 用于卡电源控制；SD 卡工作期间请勿作为 GPIO 使用 |
| GPIO45 作 GPIO | 去掉 | 常供电（栅极由 R27 下拉） | 可在 P6-40 自由使用 |

- 去掉 R30 后，软件无法再对 SD 卡断电重启。
- BSP 在挂载和卸载 SD 卡时仍会驱动 GPIO45（挂载时对卡断电重启，卸载时把 GPIO45 拉高）。如 GPIO45 另作他用，请在挂载 SD 卡后再重新配置它，或修改 BSP。

## 3.5. 使用注意事项

- GPIO7/8 I2C 总线上已有 ES8311（0x18）、GT911（0x5D/0x14）、LCD 背光控制器（0x45）和摄像头 SCCB，外接 I2C 设备时请避免地址冲突。
- GPIO6、GPIO45、GPIO53、GPIO54 是板载外设的控制信号（ESP32-C5 唤醒、microSD 电源、扬声器功放、ESP32-C5 复位），对应外设工作期间请勿单独驱动。
- GPIO28–GPIO31、GPIO34–GPIO35、GPIO49–GPIO52 用于以太网 RMII/SMI 信号和 PHY 复位，请勿作为普通 GPIO 使用。
- GPIO34–GPIO38 是 ESP32-P4 的 Strapping 引脚，在上电和复位时被采样。GPIO34/35 同时用于以太网 TXD0/TXD1，GPIO36 引出到排针 P6-24，GPIO37/38 用于烧录和日志；请确保外接电路不会在采样期间干扰其电平。
- GPIO0/GPIO1 接 32.768 kHz 晶振，作为普通 GPIO 使用前需处理晶振电路。
- LCD 复位、GT911 复位和 GT911 中断均未接线，触摸状态通过 I2C 轮询读取。
- 请在断电状态下连接屏幕/摄像头排线，并对照模组说明确认接口方向。

# 4. 相关文档

## 4.1. 数据手册与原理图

- [ESP32-P4-WIFI6-DB-POE-ETH 原理图](../../../hardware/ESP32-P4-WIFI6-DB-POE-ETH.pdf)（PDF）

- [ESP32-P4 技术规格书](https://www.espressif.com/sites/default/files/documentation/esp32-p4_datasheet_cn.pdf)（PDF）
- [ESP32-C5 技术规格书](https://www.espressif.com/sites/default/files/documentation/esp32-c5_datasheet_cn.pdf)（PDF）

## 4.2. 软件资源

- [BSP README](../../../components/esp32_p4_wifi6_db_poe_eth/README_zh.md) 和 [API 参考](../../../components/esp32_p4_wifi6_db_poe_eth/API.md)
- [Brookesia 固件](../../../firmware/brookesia/README_CN.md)
- [整板测试固件](../../../firmware/factory_test/README_CN.md)
