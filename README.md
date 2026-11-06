<div align="center">
  <h1>ESP32-P4-WIFI6-DB-POE-ETH</h1>
  <p><strong>ESP32-P4 and ESP32-C5 Wi-Fi 6 development board with PoE Ethernet</strong></p>
  <p><strong>English</strong> | <a href="README_ZH.md">简体中文</a></p>
  <p>
    <a href="https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-POE-ETH/actions/workflows/build.yml"><img alt="Build" src="https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-POE-ETH/actions/workflows/build.yml/badge.svg"></a>
    <a href="https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-POE-ETH/releases/latest"><img alt="Latest Release" src="https://img.shields.io/github/v/release/waveshareteam/ESP32-P4-WIFI6-DB-POE-ETH"></a>
    <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/waveshareteam/ESP32-P4-WIFI6-DB-POE-ETH"></a>
  </p>
  <p>
    <a href="https://www.waveshare.net/shop/ESP32-P4-WIFI6-DB-POE-ETH.htm">🌐 Product</a> ·
    <a href="firmware/">📦 Firmware</a> ·
    <a href="examples/esp-idf/">🧩 ESP-IDF</a> ·
    <a href="examples/arduino/examples/">🔧 Arduino</a> ·
    <a href="hardware/schematic/">📚 Documentation</a>
  </p>
</div>

---

## ✨ Overview

The ESP32-P4-WIFI6-DB-POE-ETH combines an ESP32-P4 application processor with an ESP32-C5 Wi-Fi 6 and Bluetooth LE coprocessor. It provides PoE Ethernet, MIPI-DSI and MIPI-CSI interfaces, audio, microSD, USB Host and GPIO expansion.

| Feature | Device / interface |
| --- | --- |
| MCU | ESP32-P4NRW32X |
| Memory | 32 MB Flash, 32 MB PSRAM |
| Wireless | ESP32-C5 over SDIO |
| Ethernet | RMII with integrated PoE module |
| Display / camera | MIPI-DSI / MIPI-CSI |
| Audio | ES8311 codec |

## 📦 Quick Start

Install ESP-IDF, connect the board over USB, and follow the [board-check example](examples/esp-idf/00_board_check/). Firmware projects are available in [`examples/factory/`](examples/factory/).

## 🧪 Examples

### ESP-IDF

| Example | Description |
| --- | --- |
| [00_board_check](examples/esp-idf/00_board_check/) | Recommended first example; prints chip, flash, and PSRAM information over serial |
| [01_i2c_tools](examples/esp-idf/01_i2c_tools/) | I2C bus scan and diagnostic console |
| [02_sdmmc](examples/esp-idf/02_sdmmc/) | microSD card access over 4-bit SDMMC |
| [03_wifistation](examples/esp-idf/03_wifistation/) | Connects to a Wi-Fi station through the ESP32-C5 `esp_hosted` link |
| [04_i2s_es8311](examples/esp-idf/04_i2s_es8311/) | ES8311 audio playback, I2S loopback, and recording |
| [05_nvs_rw_value](examples/esp-idf/05_nvs_rw_value/) | NVS read/write example with a persistent boot counter |
| [06_eth2ap](examples/esp-idf/06_eth2ap/) | Ethernet-to-Wi-Fi access point bridge |
| [07_ethernet_basic](examples/esp-idf/07_ethernet_basic/) | Basic RMII Ethernet bring-up with `esp_netif` |
| [08_mipi_dsi](examples/esp-idf/08_mipi_dsi/) | Basic MIPI-DSI display test using color bars |
| [09_lvgl](examples/esp-idf/09_lvgl/) | LVGL benchmark on the MIPI-DSI panel |
| [10_mipi_csi](examples/esp-idf/10_mipi_csi/) | Displays the MIPI-CSI camera feed on the LCD via `esp_video` and PPA |
| [11_usb_extend_screen](examples/esp-idf/11_usb_extend_screen/) | USB extended-display example |
| [12_generic_gpio](examples/esp-idf/12_generic_gpio/) | LVGL grid for viewing and toggling expansion-header GPIO levels |
| [13_board_manager](examples/esp-idf/13_board_manager/) | Interactive console for starting the board with ESP Board Manager and running LCD, touch, audio, camera, SD card, Ethernet, and GPIO tests |
| [factory_test](examples/esp-idf/factory_test/) | Interactive hardware validation console |
| [14_ethernet_iperf](examples/esp-idf/14_ethernet_iperf/) | Ethernet TCP/UDP throughput test with an iPerf 2.x PC peer |
| [15_wifi_iperf](examples/esp-idf/15_wifi_iperf/) | ESP32-C5 Wi-Fi TCP/UDP throughput test over the ESP32-P4 ESP-Hosted SDIO link, with an iPerf 2.x PC peer |
| [16_bleprph_minimal](examples/esp-idf/16_bleprph_minimal/) | Minimal NimBLE peripheral that advertises through the ESP32-C5 BLE controller over ESP-Hosted HCI |

### Arduino

Follow the [board definition installation guide](examples/arduino/libraries/board_config/README.md), then open a sketch in Arduino IDE.

| Example | Description |
| --- | --- |
| [board_check](examples/arduino/examples/board_check/) | Displays chip, flash, PSRAM, and memory information |
| [gpio](examples/arduino/examples/gpio/) | Interactive GPIO test over serial |
| [i2c](examples/arduino/examples/i2c/) | Scans the board's I2C1 bus |
| [i2s](examples/arduino/examples/i2s/) | ES8311 microphone-to-speaker audio loopback |
| [sdmmc](examples/arduino/examples/sdmmc/) | microSD card read/write test |
| [mipi_dsi](examples/arduino/examples/mipi_dsi/) | MIPI-DSI color-bar and backlight test |
| [mipi_csi](examples/arduino/examples/mipi_csi/) | Displays video from an OV5647 camera on the LCD |

## 🛠️ Development Environment

| Framework | Version in product configuration |
| --- | --- |
| ESP-IDF | `v5.5.5`, `v6.1` |
| Arduino-ESP32 | `3.3.11` |

## 📚 Resources

- [Schematic](hardware/schematic/ESP32-P4-WIFI6-DB-POE-ETH.pdf)
- [Hardware guide](docs/user-guide.md)
- [Product page](https://www.waveshare.net/shop/ESP32-P4-WIFI6-DB-POE-ETH.htm)

## 🤝 Support

- [Report an example or repository issue](https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-POE-ETH/issues)
- [Waveshare support](https://docs.waveshare.com/support)

## 📄 License

Apache License 2.0. See [LICENSE](LICENSE).
