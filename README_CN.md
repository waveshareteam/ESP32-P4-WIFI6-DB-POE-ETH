# ESP32-P4-WIFI6-DB-POE-ETH

[English](README.md)

本仓库为 **ESP32-P4-WIFI6-DB-POE-ETH** 开发板提供 ESP-IDF 板级支持包（BSP）和外设示例。
该开发板以 ESP32-P4 为主处理器，搭配 ESP32-C5 Wi-Fi 6 / Bluetooth LE 无线协处理器，
支持 RMII 以太网（可 PoE 供电）、可选 MIPI-DSI 触摸屏，以及 MIPI-CSI 摄像头接口。

## ✨ 概述

本仓库包含：

- 一个可直接使用的 BSP 组件（`components/waveshare_bsp/esp32_p4_wifi6_db_poe_eth`），提供
  显示、触摸、音频、SD 卡、以太网和扩展排针相关的 `bsp_*` API。
- 一份针对同一块板卡的
  [ESP Board Manager](https://github.com/espressif/esp-board-manager) 板卡定义
  （`components/waveshare_bmgr/esp32_p4_wifi6_db_poe_eth`），通过
  `idf.py bmgr -b esp32_p4_wifi6_db_poe_eth` 选定。
- 一组 ESP-IDF 示例（`examples/esp-idf/`），逐一验证板上各外设，从最基础的开发板
  检查到 LVGL、摄像头和 USB 演示。

完整的引脚映射和 API 说明请参阅 BSP 的
[README](components/waveshare_bsp/esp32_p4_wifi6_db_poe_eth/README_zh.md) /
[README (English)](components/waveshare_bsp/esp32_p4_wifi6_db_poe_eth/README.md) 以及
[API 参考](components/waveshare_bsp/esp32_p4_wifi6_db_poe_eth/API.md)，下表为摘要；
BSP 与 Board Manager 两种方式的关系见
[components/README_zh.md](components/README_zh.md)。

## 🖥️ 硬件概况

| 项目 | 说明 |
| --- | --- |
| 主处理器 | ESP32-P4，双核 RISC-V HP 系统加低功耗核 |
| Flash | 32 MB（QIO） |
| PSRAM | 片上 PSRAM，200 MHz |
| 无线 | ESP32-C5（Wi-Fi 6 + Bluetooth LE），通过 4-bit SDIO 连接，经 `esp_hosted` / `esp_wifi_remote` 使用 |
| 以太网 | RMII MAC/PHY（通用 IEEE 802.3 PHY 驱动），支持 PoE 供电 |
| 显示 | 可选双 lane MIPI-DSI 屏幕：HX8394（5 英寸，720×1280）、ILI9881C（7 英寸，720×1280）、JD9365（8/10.1 英寸，800×1280，默认） |
| 触摸 | GT911 电容触摸控制器，通过共享 I2C 轮询 |
| 音频 | ES8311 编解码器，扬声器输出 + 单路模拟麦克风输入 |
| 存储 | microSD，4-bit SDMMC |
| 摄像头 | MIPI-CSI，2-lane，通过 `esp_video` |
| USB | USB Host |
| 扩展 | GPIO 排针，通过 `bsp_get_header_gpios()` 枚举 |
| ESP-IDF target | `esp32p4`，要求 ESP-IDF >= 5.5 |

> **说明**
> BSP 未配置外部 RTC，也没有专用的 LCD reset GPIO；LCD 背光通过共享 I2C 总线控制，
> 而非独立 GPIO。完整引脚分配请见
> [components/waveshare_bsp/esp32_p4_wifi6_db_poe_eth/README_zh.md](components/waveshare_bsp/esp32_p4_wifi6_db_poe_eth/README_zh.md)。

## 🗂️ 仓库结构

| 路径 | 用途 |
| --- | --- |
| `examples/esp-idf/` | 本开发板的 ESP-IDF 外设示例 |
| `examples/Arduino/` | Arduino-ESP32 示例及本地板卡定义（`examples/Arduino/esp32/`） |
| `components/waveshare_bsp/esp32_p4_wifi6_db_poe_eth/` | 板级支持包（BSP）组件源码、README 和 API 参考 |
| `components/waveshare_bmgr/esp32_p4_wifi6_db_poe_eth/` | 同一板卡的 ESP Board Manager 板卡定义 |
| `hardware/` | 预留给硬件文档（目前为空） |
| `asserts/` | 预留给文档使用的图片（目前为空） |

## 🚀 快速开始

安装 ESP-IDF（>= 5.5）后，先构建开发板检查示例：

```bash
cd examples/esp-idf/00_board_check
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

将 `PORT` 替换为你的串口，例如 Windows 上的 `COM7` 或 Linux 上的 `/dev/ttyACM0`。
开发板检查通过后，可从下表选择所需示例。

## 🧪 ESP-IDF 示例

| 示例 | 说明 |
| --- | --- |
| [00_board_check](examples/esp-idf/00_board_check/) | 首个建议运行的示例，串口打印芯片、Flash、PSRAM 信息 |
| [01_i2c_tools](examples/esp-idf/01_i2c_tools/) | I2C 总线扫描与诊断控制台工具 |
| [02_sdmmc](examples/esp-idf/02_sdmmc/) | microSD 卡的 4-bit SDMMC 读写访问 |
| [03_wifistation](examples/esp-idf/03_wifistation/) | 通过 ESP32-C5 `esp_hosted` 链路连接 Wi-Fi station |
| [04_i2s_es8311](examples/esp-idf/04_i2s_es8311/) | ES8311 音频播放，以及 I2S 回声/录音 |
| [05_nvs_rw_value](examples/esp-idf/05_nvs_rw_value/) | NVS 读写示例，持久化开机计数器 |
| [06_eth2ap](examples/esp-idf/06_eth2ap/) | 以太网转 Wi-Fi AP 桥接 |
| [07_ethernet_basic](examples/esp-idf/07_ethernet_basic/) | 基于 `esp_netif` 的 RMII 以太网基础示例 |
| [08_mipi_dsi](examples/esp-idf/08_mipi_dsi/) | 最简单的 MIPI-DSI 显示点亮示例：彩条测试图案 |
| [09_lvgl](examples/esp-idf/09_lvgl/) | MIPI-DSI 屏幕上的 LVGL 性能测试演示 |
| [10_mipi_csi](examples/esp-idf/10_mipi_csi/) | 通过 `esp_video` 和 PPA 将 MIPI-CSI 摄像头画面显示到 LCD |
| [11_usb_extend_screen](examples/esp-idf/11_usb_extend_screen/) | USB 扩展屏示例 |
| [12_generic_gpio](examples/esp-idf/12_generic_gpio/) | LVGL 网格界面，实时显示/切换扩展排针 GPIO 电平 |
| [13_board_manager](examples/esp-idf/13_board_manager/) | 通过 ESP Board Manager 启动开发板的交互式控制台，可在真实硬件上运行 LCD、触摸、音频、摄像头、SD 卡、以太网、GPIO 等功能测试用例 |
| [14_ethernet_iperf](examples/esp-idf/14_ethernet_iperf/) | 使用 PC 端 iPerf 2.x 测量以太网 TCP/UDP 吞吐 |

## 📡 以太网与 Wi-Fi

- 以太网使用内部 EMAC 配合通用 IEEE 802.3 PHY 驱动（MDC/MDIO/PHY reset 分别为
  GPIO31/52/51）。BSP 只安装驱动，netif、事件循环以及 DHCP/静态地址均由应用管理，
  详见 [以太网生命周期](components/waveshare_bsp/esp32_p4_wifi6_db_poe_eth/README_zh.md#接入应用)。
- Wi-Fi 与 Bluetooth LE 由 ESP32-C5 协处理器通过 4-bit SDIO（slot 1）提供，经应用的
  `esp_hosted` / `esp_wifi_remote` 组件使用，可参考
  [03_wifistation](examples/esp-idf/03_wifistation/) 和
  [06_eth2ap](examples/esp-idf/06_eth2ap/)。BSP 本身不建立第二套 Wi-Fi 传输实例。

## 📄 许可证

本仓库基于 Apache License 2.0 授权，详情请见 [LICENSE](LICENSE)。
