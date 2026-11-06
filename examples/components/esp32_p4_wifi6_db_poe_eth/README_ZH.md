# ESP32-P4-WIFI6-DB-POE-ETH BSP

[English](README.md) · [API](API.md)

本组件由原 BSP 适配而来，组件版本为 `0.0.1`，清单要求 ESP-IDF >= 5.5、目标 ESP32-P4。
I2C、SDMMC、ES8311 和网络配置以本仓库 `examples/esp-idf` 的源码及已有生成配置为依据。
网络示例已有构建配置使用 ESP-IDF 6.0.1；新增以太网 API 也已核对本机 5.5.4 头文件。

## 外设配置

| 外设 | 信号 | GPIO / 配置 |
| --- | --- | --- |
| I2C | SDA / SCL | 7 / 8 |
| ES8311 | MCLK / BCLK / WS | 13 / 12 / 10 |
| ES8311 | MCU DOUT / DIN | 9 / 11 |
| 功放 | 使能 | 53，高有效 |
| SDMMC | CLK / CMD | 43 / 44 |
| SDMMC | D0 / D1 / D2 / D3 | 39 / 40 / 41 / 42 |
| SD 卡 | 电源使能 / IO 电源 | GPIO45 低有效 / LDO4 |
| SD 卡 | 主机 | slot 0、4-bit、40 MHz |
| 以太网 | MDC / MDIO / PHY reset | 31 / 52 / 51 |
| 以太网 | PHY 地址 | 1 |
| RMII | 50 MHz 参考时钟输入 | GPIO50 |
| RMII | TX_EN / TXD0 / TXD1 | 49 / 34 / 35 |
| RMII | CRS_DV / RXD0 / RXD1 | 28 / 29 / 30 |
| ESP32-C5 SDIO | CLK / CMD | 18 / 19 |
| ESP32-C5 SDIO | D0 / D1 / D2 / D3 | 14 / 15 / 16 / 17 |
| ESP32-C5 | 复位 | GPIO54，低有效 |
| ESP32-C5 SDIO | 主机 | slot 1、4-bit、40 MHz |

音频保留默认 48 kHz、16-bit、单声道全双工配置和单模拟麦克风路径，MCLK 默认 256 倍频。
`bsp_audio_init()` 可以接收应用自己的 I2S 配置。保留 `no_dac_ref=true`，双通道录音的右声道不填入 DAC 回采。

SD 卡挂载前按 `02_sdmmc` 的时序，将 GPIO45 拉高 100 ms 再拉低。
重复挂载返回 `ESP_ERR_INVALID_STATE`，避免给已挂载的卡断电。默认不在挂载失败时格式化。
保留 SDMMC 主机复用逻辑，SD 卡使用 slot 0，Hosted 使用 slot 1。

## 保留的屏幕功能

| 屏幕 | 驱动 | 分辨率 | 每条 DSI lane 速率 |
| --- | --- | --- | --- |
| 5-DSI-TOUCH-A | HX8394 | 720×1280 | 700 Mbps |
| 7-DSI-TOUCH-A | ILI9881C | 720×1280 | 1000 Mbps |
| 8-DSI-TOUCH-A | JD9365 | 800×1280 | 1500 Mbps |
| 10.1-DSI-TOUCH-A | JD9365 | 800×1280 | 1500 Mbps |

四种原有屏幕均保留，默认 10.1 英寸，通过 Kconfig 选择；均为双 lane。
保留 RGB565/RGB888、1～3 帧缓冲、LVGL adapter 和原有 `bsp_display_*` API。
背光使用共享 I2C 的 `0x45` 地址、`0x96` 寄存器，将 0～100% 映射为 0～255。
LCD reset 保持 NC；GT911 reset/interrupt 保持 NC，使用轮询，尝试 `0x5D` 和 `0x14`。

USB Host、CSI 摄像头、SPIFFS 代码保留。当前外设示例没有验证 USB/CSI 的板级接线与运行情况。
I2C/I2S 控制器仍可配置；I2C 支持 100/400 kHz，SD 卡和 SPIFFS 挂载选项保持可配置。

## 接入应用

在应用包含 `project.cmake` 前，把本目录加入 `EXTRA_COMPONENT_DIRS`。
统一使用 `#include "bsp/esp-bsp.h"`，或板级头文件 `bsp/esp32_p4_wifi6_db_poe_eth.h`。
原来的 Nano 专用文件名已替换，已有 `bsp_*` 显示、音频、存储 API 保留。

以太网启用 `CONFIG_ETH_USE_ESP32_EMAC`，调用 `bsp_eth_init(&handle)` 安装 MAC/PHY。
使用与 `07_ethernet_basic` 相同的 generic IEEE 802.3 PHY 驱动；`06_eth2ap` 的配置选用 IP101。
应用负责事件循环、netif/glue、DHCP 或静态地址，以及 `esp_eth_start()`。
释放时先停止驱动、释放 glue 和 netif，再调用 `bsp_eth_deinit()`；驱动卸载失败不会释放 MAC/PHY。
不要与示例原有 Ethernet initializer 同时初始化同一个 EMAC。关闭 EMAC 时，合法参数调用返回 `ESP_ERR_NOT_SUPPORTED`。
完整接入代码见 [英文 README](README.md#ethernet-lifecycle)。

Wi-Fi 沿用应用的 `esp_hosted` 和 `esp_wifi_remote`，不在 BSP 内建立第二套传输实例。
现有 `06_eth2ap` 锁文件解析到 Hosted 3.0.7、Wi-Fi Remote 1.6.4；本次没有安装包或修改依赖版本。
Hosted 配置由应用管理，参考 `03_wifistation` 和 `06_eth2ap`，需核对最终生成配置确实选中 C5、slot 1 和上表引脚。
参考 Hosted 版本中，低有效复位选项会生成 `CONFIG_ESP_HOSTED_HOST_RESET_ACTIVE_LOW=y`。
C5 需要匹配的 Hosted 固件；STA/AP、凭据和桥接策略由应用配置，参考 `03_wifistation` 和 `06_eth2ap`。

## 扩展排针

`header_gpios` 数组按要求保持不动：

```text
23, 5, 20, 21, 25, 26, 32, 4, 22, 24, 27, 33, 36, 3, 2, 54,
47, 46, 45, 6, 53, 48
```

该数组不是“当前空闲引脚”清单。GPIO45 控制 SD 卡电源、GPIO53 控制功放、GPIO54 复位 C5，
相关外设工作时不要独立驱动这些引脚。

## 验证范围

本次仅进行源码、声明、配置和差异静态检查，未编译、烧录或进行硬件测试。
显示、触摸、音频、SD 卡、以太网、Wi-Fi、USB 和 CSI 仍需各自的构建与硬件验证。
