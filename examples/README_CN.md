# 示例

[English](README.md)

本仓库为 ESP32-P4-WIFI6-DB-POE-ETH 开发板提供 [ESP-IDF](esp-idf/) 和
[Arduino](Arduino/) 两套示例。

## ESP-IDF 示例

ESP-IDF 工程位于 [esp-idf](esp-idf/) 目录，每个子目录都是一个独立的
ESP-IDF 工程。

```bash
cd examples/esp-idf/00_board_check
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

这些示例围绕本板可选的 MIPI-DSI 显示屏、GT911 触摸控制器、ESP32-C5
Wi-Fi 6/BLE 协处理器、支持 PoE 供电的 RMII 以太网、SD 卡、音频编解码器、
USB 以及 MIPI-CSI 摄像头组织。

### 建议的运行顺序

1. `00_board_check`：确认工具链、烧录流程、串口监视器、Flash 和 PSRAM。
2. `01_i2c_tools`、`05_nvs_rw_value`：在不需要显示屏或网络硬件的情况下，
   了解基本的外设/运行时用法。
3. `02_sdmmc`、`04_i2s_es8311`：验证存储和音频。
4. `08_mipi_dsi`、`09_lvgl`、`12_generic_gpio`：验证显示和触摸。
5. `03_wifistation`、`06_eth2ap`、`07_ethernet_basic`：验证无线和有线网络；随后使用
   `14_ethernet_iperf` 测量以太网吞吐。
6. `10_mipi_csi`、`11_usb_extend_screen`：在连接好对应摄像头或 USB 硬件后
   使用。
7. `13_board_manager`：不走 BSP，而是通过 ESP Board Manager
   （`components/waveshare_bmgr`）初始化开发板的交互式控制台，可对声明的每个设备
   运行功能测试用例。

### ESP-IDF 示例索引

| 目录 | 作用 | 硬件说明 |
| --- | --- | --- |
| [00_board_check](esp-idf/00_board_check/) | 首次运行的开发板和工具链检查 | 仅需 USB |
| [01_i2c_tools](esp-idf/01_i2c_tools/) | I2C 扫描与诊断工具 | 便于触摸/编解码器/摄像头的 bring-up |
| [02_sdmmc](esp-idf/02_sdmmc/) | microSD 卡访问 | 需要 microSD 卡 |
| [03_wifistation](esp-idf/03_wifistation/) | Wi-Fi station | 使用 ESP32-C5 ESP-Hosted 链路 |
| [04_i2s_es8311](esp-idf/04_i2s_es8311/) | I2S 音频编解码器 | 使用板载 ES8311 编解码器 |
| [05_nvs_rw_value](esp-idf/05_nvs_rw_value/) | NVS 读写，开机计数器 | 仅需 USB |
| [06_eth2ap](esp-idf/06_eth2ap/) | 以太网转 Wi-Fi AP 桥接 | 需要以太网线和 ESP32-C5 Wi-Fi 链路 |
| [07_ethernet_basic](esp-idf/07_ethernet_basic/) | 基础 RMII 以太网 bring-up | 需要以太网线 |
| [08_mipi_dsi](esp-idf/08_mipi_dsi/) | MIPI-DSI 彩条 | 需要 BSP 支持的 MIPI-DSI 屏幕 |
| [09_lvgl](esp-idf/09_lvgl/) | LVGL 性能测试 | 需要 BSP 支持的 MIPI-DSI 屏幕 |
| [10_mipi_csi](esp-idf/10_mipi_csi/) | MIPI-CSI 摄像头画面显示到 LCD | 需要摄像头模块和 MIPI-DSI 屏幕 |
| [11_usb_extend_screen](esp-idf/11_usb_extend_screen/) | USB 扩展屏 | 需要 Windows 端驱动 |
| [12_generic_gpio](esp-idf/12_generic_gpio/) | 扩展排针 GPIO 监视器（LVGL） | 需要带 GT911 触摸的 MIPI-DSI 屏幕 |
| [13_board_manager](esp-idf/13_board_manager/) | ESP Board Manager bring-up 控制台与测试用例 | 使用 `components/waveshare_bmgr`，编译前需安装 `esp-bmgr-assist` 并执行 `idf.py bmgr`；各用例需要对应的屏幕 / 卡 / 网线 |
| [14_ethernet_iperf](esp-idf/14_ethernet_iperf/) | 以太网 TCP/UDP 吞吐测试 | 需要以太网线、DHCP 网络和运行 iPerf 2.x 的 PC |

## Arduino 示例

[Arduino 示例](Arduino/) 使用 Arduino-ESP32 `3.3.11`，配合
[Arduino/esp32](Arduino/esp32/) 下本地提供的 `Waveshare
ESP32-P4-WIFI6-DB-POE-ETH` 板卡定义。涵盖开发板检查、GPIO、I2C、I2S 音频、
microSD 以及 MIPI-DSI/MIPI-CSI 显示等示例。板卡安装方法见
[Arduino 板卡 README](Arduino/esp32/README.md)，各示例所需的板卡菜单设置
和硬件见[对应示例自己的 README](Arduino/)。

生成产物（`build/`、`managed_components/`、`dependencies.lock` 以及本地
`sdkconfig`）已被忽略，不应提交。
