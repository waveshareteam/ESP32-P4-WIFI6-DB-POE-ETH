<div align="center">
  <h1>ESP32-P4-WIFI6-DB-POE-ETH</h1>
  <p><strong>搭载 ESP32-P4 和 ESP32-C5 的 Wi-Fi 6 开发板，集成 PoE 以太网</strong></p>
  <p><a href="README.md">English</a> | <strong>简体中文</strong></p>
  <p>
    <a href="https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-POE-ETH/actions/workflows/build.yml"><img alt="Build" src="https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-POE-ETH/actions/workflows/build.yml/badge.svg"></a>
    <a href="https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-POE-ETH/releases/latest"><img alt="Latest Release" src="https://img.shields.io/github/v/release/waveshareteam/ESP32-P4-WIFI6-DB-POE-ETH"></a>
    <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/waveshareteam/ESP32-P4-WIFI6-DB-POE-ETH"></a>
  </p>
  <p>
    <a href="https://www.waveshare.net/shop/ESP32-P4-WIFI6-DB-POE-ETH.htm">🌐 产品</a> ·
    <a href="https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-POE-ETH/releases/latest">📦 固件</a> ·
    <a href="examples/esp-idf/">🧩 ESP-IDF</a> ·
    <a href="examples/arduino/examples/">🔧 Arduino</a> ·
    <a href="hardware/">📚 文档</a>
  </p>
</div>

---

## ✨ 概述

ESP32-P4-WIFI6-DB-POE-ETH 以 ESP32-P4 为主控，通过 ESP32-C5 提供 Wi-Fi 6 和 Bluetooth LE。开发板提供 PoE 以太网、MIPI-DSI、MIPI-CSI、音频、microSD、USB Host 和 GPIO 扩展接口。

| 功能 | 器件或接口 |
| --- | --- |
| 主控 | ESP32-P4NRW32X |
| 存储器 | 32 MB Flash、32 MB PSRAM |
| 无线 | ESP32-C5，通过 SDIO 连接 |
| 以太网 | RMII，集成 PoE 模块 |
| 显示 / 摄像头 | MIPI-DSI / MIPI-CSI |
| 音频 | ES8311 编解码器 |
| 板级支持 | 本地 BSP [`esp32_p4_wifi6_db_poe_eth`](examples/components/esp32_p4_wifi6_db_poe_eth/)，Board Manager 板卡定义 `waveshare/waveshare_boards` |

## 📦 快速开始

体验示例最快的方法是使用[最新版本](https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-POE-ETH/releases/latest)中打包好的可烧录固件。

1. 下载所需示例和框架版本对应的 `*-combined.zip`。
2. 解压后通过 `python -m pip install esptool` 安装 esptool。
3. 通过 USB 连接开发板。
4. Windows 运行 `flash_combined.bat COMx`，Linux 运行 `./flash_combined.sh /dev/ttyACM0`。
5. 如果开发板未自动重启，请手动复位。

出厂固件（恢复开箱演示）：在 [`firmware`](firmware/) 目录下载文件名带 `FactoryOnly` 的文件。

## 🧪 示例

### ESP-IDF

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
| [15_wifi_iperf](examples/esp-idf/15_wifi_iperf/) | 通过 ESP32-P4NRW32X ESP-Hosted SDIO 链路测量 ESP32-C5 Wi-Fi TCP/UDP 吞吐，需要 PC 端 iPerf 2.x |
| [16_bleprph_minimal](examples/esp-idf/16_bleprph_minimal/) | 最小 NimBLE 外设示例，通过 ESP-Hosted HCI 使用 ESP32-C5 的 BLE 控制器进行广播 |
| [factory_test](examples/esp-idf/factory_test/README_ZH.md) | 交互式硬件验证控制台 |

### Arduino

按[板卡定义安装说明](examples/arduino/libraries/board_config/README.md)配置 Arduino IDE，再打开示例。

| 示例 | 说明 |
| --- | --- |
| [board_check](examples/arduino/examples/board_check/) | 查看芯片、Flash、PSRAM 和内存信息 |
| [gpio](examples/arduino/examples/gpio/) | 通过串口交互测试 GPIO |
| [i2c](examples/arduino/examples/i2c/) | 扫描开发板 I2C1 总线 |
| [i2s](examples/arduino/examples/i2s/) | ES8311 麦克风到扬声器的音频回环 |
| [sdmmc](examples/arduino/examples/sdmmc/) | microSD 卡读写 |
| [mipi_dsi](examples/arduino/examples/mipi_dsi/) | MIPI-DSI 屏幕色条和背光测试 |
| [mipi_csi](examples/arduino/examples/mipi_csi/) | 将 OV5647 摄像头画面显示到 LCD |

## 🛠️ 开发环境

| 开发框架 | 版本 | 示例数 |
| --- | --- | ---: |
| ESP-IDF | `v5.5.6`、`v6.1` | 18 |
| Arduino-ESP32 | `3.3.11` | 7 |

## 📚 资料

- [原理图](hardware/schematic/)
- [硬件参考指南](docs/user-guide_ZH.md)
- [产品页面](https://www.waveshare.net/shop/ESP32-P4-WIFI6-DB-POE-ETH.htm)

## 🤝 支持

- [反馈示例或仓库问题](https://gitee.com/waveshare/ESP32-P4-WIFI6-DB-POE-ETH/issues)
- [微雪技术支持](https://docs.waveshare.net/support)

## 📄 许可证

Apache License 2.0，详见 [LICENSE](LICENSE)。
