<div align="center">
  <h1>ESP32-P4-WIFI6-DB-POE-ETH</h1>
  <p><strong>搭载 ESP32-P4NRW32X 和 ESP32-C5 Wi-Fi 6 协处理器，集成 PoE 以太网，提供 MIPI-DSI / MIPI-CSI 接口的开发板</strong></p>
  <p>
    <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/waveshareteam/ESP32-P4-WIFI6-DB-POE-ETH"></a>
  </p>
  <p>
    <a href="README.md">English</a> ·
    <a href="https://www.waveshare.net/shop/ESP32-P4-WIFI6-DB-POE-ETH.htm">🌐 产品页面</a> ·
    <a href="https://docs.waveshare.net/ESP32-P4-WIFI6-DB-POE-ETH-DB">📚 文档</a> ·
    <a href="firmware/">📦 固件</a> ·
    <a href="examples/esp-idf/">🧩 ESP-IDF 示例</a> ·
    <a href="examples/Arduino/">🔧 Arduino 示例</a>
  </p>
</div>

---

## ✨ 概述

本仓库提供 ESP-IDF 板级支持包（BSP）、ESP Board Manager 板卡定义、ESP-IDF 与 Arduino
示例，以及面向本开发板的整板工厂测试固件源码。

ESP32-C5 通过 SDIO 连接主处理器，并提供 Wi-Fi 6 和 Bluetooth LE。板上提供
双 lane MIPI-DSI 显示与触摸接口、MIPI-CSI 摄像头接口、ES8311 音频、microSD、USB Host、
GPIO 扩展排针和 RMII 以太网。板载已集成 PoE 模块；使用 PoE 供电时，需连接支持
IEEE 802.3af PoE 标准的路由器或交换机。

完整的引脚映射和 API 说明请参阅 BSP 的
[README](components/esp32_p4_wifi6_db_poe_eth/README_zh.md) /
[README (English)](components/esp32_p4_wifi6_db_poe_eth/README.md) 和
[API 参考](components/esp32_p4_wifi6_db_poe_eth/API.md)。

## 🖥️ 硬件概况

| 项目 | 说明 |
| --- | --- |
| 主处理器 | ESP32-P4NRW32X，双核 RISC-V HP 系统加低功耗核 |
| Flash | 32 MB（QIO） |
| PSRAM | 片上 PSRAM，200 MHz |
| 无线 | ESP32-C5 协处理器，通过 SDIO 上的 ESP-Hosted 提供 Wi-Fi 6 和蓝牙 |
| 以太网 | RMII MAC/PHY（通用 IEEE 802.3 PHY 驱动）；板载集成 PoE 模块 |
| 显示 | 双通道 MIPI-DSI；支持外接 Waveshare [5](https://www.waveshare.net/shop/5-DSI-TOUCH-A.htm)、[7](https://www.waveshare.net/shop/7-DSI-TOUCH-A.htm)、[8](https://www.waveshare.net/shop/8-DSI-TOUCH-A.htm)、[10.1](https://www.waveshare.net/shop/10.1-DSI-TOUCH-A.htm) 英寸面板 |
| 触摸 | GT911 电容触摸控制器，通过共享 I2C 轮询 |
| 音频 | ES8311 编解码器，扬声器输出 + 单路模拟麦克风输入 |
| 存储 | microSD，4-bit SDMMC |
| 摄像头 | MIPI-CSI，2-lane，通过 `esp_video` |
| USB | USB Host |
| 扩展 | 通过 GPIO 排针连接外部模块 |
| ESP-IDF target | `esp32p4`，要求 ESP-IDF >= 5.5 |

### ESP-IDF LCD 选择

| 面板 | 控制器 | 分辨率 |
| --- | --- | --- |
| [Waveshare 5-DSI-TOUCH-A](https://www.waveshare.net/shop/5-DSI-TOUCH-A.htm) | HX8394，5 英寸 | 720 × 1280 |
| [Waveshare 7-DSI-TOUCH-A](https://www.waveshare.net/shop/7-DSI-TOUCH-A.htm) | ILI9881C，7 英寸 | 720 × 1280 |
| [Waveshare 8-DSI-TOUCH-A](https://www.waveshare.net/shop/8-DSI-TOUCH-A.htm) | JD9365，8 英寸 | 800 × 1280 |
| [Waveshare 10.1-DSI-TOUCH-A](https://www.waveshare.net/shop/10.1-DSI-TOUCH-A.htm) | JD9365，10.1 英寸 | 800 × 1280 |

构建前请确认所选面板与实际连接的面板匹配。

## 🚀 快速开始

安装 ESP-IDF（>= 5.5）后，按以下步骤运行开发板检查示例：

```bash
cd examples/esp-idf/00_board_check
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

将 `PORT` 替换为实际串口号，例如 Windows 上的 `COM7` 或 Linux 上的 `/dev/ttyACM0`。
开发板检查通过后，可从下表选择要运行的示例。

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
| [15_wifi_iperf](examples/esp-idf/15_wifi_iperf/) | 通过 ESP32-P4NRW32X ESP-Hosted SDIO 链路测量 ESP32-C5 Wi-Fi TCP/UDP 吞吐，需要 PC 端 iPerf 2.x |

## 🧪 Arduino 示例

Arduino 示例位于 `examples/Arduino/`。请先按[本地板卡文件说明](examples/Arduino/esp32/README.md)
安装板卡定义，再在 Arduino IDE 中选择 `Waveshare ESP32-P4-WIFI6-DB-POE-ETH`。

| 示例 | 说明 |
| --- | --- |
| [board_check](examples/Arduino/board_check/) | 查看芯片、Flash、PSRAM 和内存信息 |
| [gpio](examples/Arduino/gpio/) | 通过串口交互测试 GPIO |
| [i2c](examples/Arduino/i2c/) | 扫描开发板 I2C1 总线 |
| [i2s](examples/Arduino/i2s/) | ES8311 麦克风到扬声器的音频回环 |
| [sdmmc](examples/Arduino/sdmmc/) | microSD 卡读写 |
| [mipi_dsi](examples/Arduino/mipi_dsi/) | MIPI-DSI 屏幕色条和背光测试 |
| [mipi_csi](examples/Arduino/mipi_csi/) | 将 OV5647 摄像头画面显示到 LCD |

## 📡 ESP32-P4NRW32X 与 ESP32-C5 无线通信

ESP32-P4NRW32X 本身不集成 Wi-Fi 或蓝牙功能。板载 ESP32-C5 通过 4-bit SDIO 与主控连接，提供
Wi-Fi 6 和 Bluetooth LE。无线功能需要 P4 Host 应用与 C5 协处理器固件配合运行；烧录 P4
应用不会更新 C5 固件。使用时请确认两侧的 ESP-Hosted 配置相互兼容。示例见
[03_wifistation](examples/esp-idf/03_wifistation/) 和
[15_wifi_iperf](examples/esp-idf/15_wifi_iperf/)；后者包含 P4 Host 和 C5 协处理器工程。

## 🌐 以太网

- 以太网由内部 EMAC 和通用 IEEE 802.3 PHY 驱动提供，MDC、MDIO 和 PHY 复位信号分别连接 GPIO31、GPIO52 和 GPIO51。
  BSP 负责安装驱动；netif、事件循环以及 DHCP 或静态地址由应用管理。详见
  [以太网生命周期](components/esp32_p4_wifi6_db_poe_eth/README_zh.md#接入应用)。

## 📦 固件

[`firmware/factory_test/`](firmware/factory_test/) 提供本开发板的整板硬件测试固件，通过串口控制台
测试 I2C、存储、网络、USB、显示、触摸、摄像头、音频和 GPIO。操作说明见[固件 README](firmware/factory_test/README_CN.md)
和[测试手册](firmware/factory_test/docs/test-procedure_CN.md)。

仓库中的 [`firmware/brookesia/`](firmware/brookesia/) 面向 ESP32-P4-WIFI6-DB-POE-ETH，是本开发板的固件工程。

## 🗂️ 仓库结构

| 路径 | 用途 |
| --- | --- |
| `examples/esp-idf/` | 本开发板的 ESP-IDF 外设示例 |
| `examples/Arduino/` | Arduino-ESP32 示例及本地板卡定义（`examples/Arduino/esp32/`） |
| `components/esp32_p4_wifi6_db_poe_eth/` | 板级支持包（BSP）组件源码、README 和 API 参考 |
| `components/waveshare_boards/esp32_p4_wifi6_db_poe_eth/` | 同一板卡的 ESP Board Manager 板卡定义 |
| `firmware/factory_test/` | 本开发板的整板硬件测试固件 |
| `firmware/brookesia/` | ESP32-P4-WIFI6-DB-POE-ETH 固件工程 |
| `docs/` | 中英文硬件参考指南 |
| `hardware/` | 硬件文档与资料 |

## 📚 文档

- [硬件参考指南](docs/zh_CN/hw-reference/user-guide-esp32-p4-wifi6-db-poe-eth.md)：板载资源、GPIO 功能和接口信息。
- [BSP 中文说明](components/esp32_p4_wifi6_db_poe_eth/README_zh.md)与
  [API 参考](components/esp32_p4_wifi6_db_poe_eth/API.md)。
- [Arduino 板卡文件说明](examples/Arduino/esp32/README.md)：本地板卡定义和安装方式。
- [工厂测试固件说明](firmware/factory_test/README_CN.md)与
  [测试操作手册](firmware/factory_test/docs/test-procedure_CN.md)。

## 🤝 支持与贡献

欢迎通过 [GitHub Issues](https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-POE-ETH/issues) 反馈问题或提交改进建议。
请附上板卡版本、连接的屏幕或摄像头、ESP-IDF 或 Arduino-ESP32 版本、复现步骤、预期与实际现象，
以及相关日志。

## 📄 许可证

本仓库基于 Apache License 2.0 授权，详情请见 [LICENSE](LICENSE)。
