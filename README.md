<div align="center">
  <h1>ESP32-P4-WIFI6-DB-POE-ETH</h1>
  <p><strong>ESP32-P4NRW32X development board with an ESP32-C5 Wi-Fi 6 coprocessor, integrated PoE Ethernet, and MIPI-DSI / MIPI-CSI interfaces</strong></p>
  <p>
    <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/waveshareteam/ESP32-P4-WIFI6-DB-POE-ETH"></a>
  </p>
  <p>
    <a href="README_CN.md">简体中文</a> ·
    <a href="docs/en/hw-reference/user-guide-esp32-p4-wifi6-db-poe-eth.md">📚 Documentation</a> ·
    <a href="firmware/">📦 Firmware</a> ·
    <a href="examples/esp-idf/">🧩 ESP-IDF Examples</a> ·
    <a href="examples/Arduino/">🔧 Arduino Examples</a>
  </p>
</div>

---

## ✨ Overview

This repository provides an ESP-IDF board support package (BSP), an ESP Board Manager
board definition, ESP-IDF and Arduino examples, and the source for the board-wide
factory-test firmware.

The ESP32-C5 connects to the main processor over SDIO and provides Wi-Fi 6 and
Bluetooth LE. The board includes a two-lane MIPI-DSI display and touch interface,
a MIPI-CSI camera interface, ES8311 audio, microSD, USB Host, a GPIO expansion
header, and RMII Ethernet. A PoE module is integrated on the board. To power it
over PoE, connect it to a router or switch that supports IEEE 802.3af.

For the complete pin assignment and board-support details, see the BSP
[README](components/esp32_p4_wifi6_db_poe_eth/README.md), [README (中文)](components/esp32_p4_wifi6_db_poe_eth/README_zh.md),
and [API reference](components/esp32_p4_wifi6_db_poe_eth/API.md).

## 🖥️ Hardware Overview

| Item | Details |
| --- | --- |
| Main processor | ESP32-P4NRW32X, dual-core RISC-V high-performance cores plus a low-power core |
| Flash | 32 MB (QIO) |
| PSRAM | On-chip PSRAM, 200 MHz |
| Wireless | ESP32-C5 (Wi-Fi 6 + Bluetooth LE) over 4-bit SDIO, used through `esp_hosted` / `esp_wifi_remote` |
| Ethernet | RMII MAC/PHY (generic IEEE 802.3 PHY driver); integrated PoE module |
| Display | Selectable two-lane MIPI-DSI panel: HX8394 (5-inch, 720 × 1280), ILI9881C (7-inch, 720 × 1280), or JD9365 (8/10.1-inch, 800 × 1280; default configuration is 10.1-inch) |
| Touch | GT911 capacitive touch controller; touch data is polled over shared I2C |
| Audio | ES8311 codec, speaker output and one analog microphone input |
| Storage | microSD over 4-bit SDMMC |
| Camera | Two-lane MIPI-CSI, via `esp_video` |
| USB | USB Host |
| Expansion | GPIO header for connecting external modules |
| ESP-IDF target | `esp32p4`; requires ESP-IDF >= 5.5 |

### ESP-IDF LCD Options

| Panel | Controller | Resolution |
| --- | --- | --- |
| [Waveshare 5-DSI-TOUCH-A](https://www.waveshare.net/shop/5-DSI-TOUCH-A.htm) | HX8394, 5-inch | 720 × 1280 |
| [Waveshare 7-DSI-TOUCH-A](https://www.waveshare.net/shop/7-DSI-TOUCH-A.htm) | ILI9881C, 7-inch | 720 × 1280 |
| [Waveshare 8-DSI-TOUCH-A](https://www.waveshare.net/shop/8-DSI-TOUCH-A.htm) | JD9365, 8-inch | 800 × 1280 |
| [Waveshare 10.1-DSI-TOUCH-A](https://www.waveshare.net/shop/10.1-DSI-TOUCH-A.htm) | JD9365, 10.1-inch | 800 × 1280 |

Before building, make sure the selected panel matches the one connected to the board.

## 🚀 Getting Started

Install ESP-IDF (>= 5.5), then build and run the board-check example:

```bash
cd examples/esp-idf/00_board_check
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

Replace `PORT` with your serial port, such as `COM7` on Windows or `/dev/ttyACM0` on Linux. After the board check succeeds, choose an example from the table below.

## 🧪 ESP-IDF Examples

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
| [14_ethernet_iperf](examples/esp-idf/14_ethernet_iperf/) | Ethernet TCP/UDP throughput test with an iPerf 2.x PC peer |
| [15_wifi_iperf](examples/esp-idf/15_wifi_iperf/) | ESP32-C5 Wi-Fi TCP/UDP throughput test over the ESP32-P4 ESP-Hosted SDIO link, with an iPerf 2.x PC peer |

## 🧪 Arduino Examples

Arduino examples are in `examples/Arduino/`. Follow the
[local board definition and installation guide (Chinese)](examples/Arduino/esp32/README.md),
then select `Waveshare ESP32-P4-WIFI6-DB-POE-ETH` in the Arduino IDE.

| Example | Description |
| --- | --- |
| [board_check](examples/Arduino/board_check/) | Displays chip, flash, PSRAM, and memory information |
| [gpio](examples/Arduino/gpio/) | Interactive GPIO test over serial |
| [i2c](examples/Arduino/i2c/) | Scans the board's I2C1 bus |
| [i2s](examples/Arduino/i2s/) | ES8311 microphone-to-speaker audio loopback |
| [sdmmc](examples/Arduino/sdmmc/) | microSD card read/write test |
| [mipi_dsi](examples/Arduino/mipi_dsi/) | MIPI-DSI color-bar and backlight test |
| [mipi_csi](examples/Arduino/mipi_csi/) | Displays video from an OV5647 camera on the LCD |

## 📡 ESP32-P4 and ESP32-C5 Wireless

The ESP32-P4 does not have built-in Wi-Fi or Bluetooth. The on-board ESP32-C5
connects to it over 4-bit SDIO and provides Wi-Fi 6 and Bluetooth LE. Wireless
operation requires a P4 host application and compatible firmware on the C5
coprocessor; flashing the P4 application does not update the C5 firmware. Keep the
ESP-Hosted configurations on both sides compatible. See
[03_wifistation](examples/esp-idf/03_wifistation/) and
[15_wifi_iperf](examples/esp-idf/15_wifi_iperf/); the latter includes separate P4
host and C5 coprocessor projects.

## 🌐 Ethernet

The Ethernet interface uses the internal EMAC with a generic IEEE 802.3 PHY driver.
MDC, MDIO, and PHY reset connect to GPIO31, GPIO52, and GPIO51, respectively. The
BSP installs the driver; the application manages the network interface, event
loop, and DHCP or static IP configuration. See
[Ethernet lifecycle](components/esp32_p4_wifi6_db_poe_eth/README.md#ethernet-lifecycle).

## 📦 Firmware

[`firmware/factory_test/`](firmware/factory_test/) contains serial-console firmware
for testing the board's I2C, storage, network, USB, display, touch, camera, audio,
and GPIO functions. See the [factory-test firmware README](firmware/factory_test/README.md)
and the [test procedure (Chinese)](firmware/factory_test/docs/test-procedure_CN.md).

[`firmware/brookesia/`](firmware/brookesia/) contains firmware for the ESP32-P4-WIFI6-DB-POE-ETH development board.

## 🗂️ Repository Layout

| Path | Purpose |
| --- | --- |
| `examples/esp-idf/` | ESP-IDF peripheral examples for this board |
| `examples/Arduino/` | Arduino-ESP32 examples and the local board definition in `examples/Arduino/esp32/` |
| `components/esp32_p4_wifi6_db_poe_eth/` | BSP component source, README, and API reference |
| `components/waveshare_boards/esp32_p4_wifi6_db_poe_eth/` | ESP Board Manager board definition |
| `firmware/factory_test/` | Whole-board hardware test firmware |
| `firmware/brookesia/` | Firmware project for the ESP32-P4-WIFI6-DB-POE-ETH |
| `docs/` | Chinese and English hardware reference guides |
| `hardware/` | Hardware documentation and resources |

## 📚 Documentation

- [Hardware reference guide](docs/en/hw-reference/user-guide-esp32-p4-wifi6-db-poe-eth.md): onboard resources, GPIO functions, and interfaces.
- [BSP README](components/esp32_p4_wifi6_db_poe_eth/README.md) and [API reference](components/esp32_p4_wifi6_db_poe_eth/API.md).
- [Arduino board definition and installation guide (Chinese)](examples/Arduino/esp32/README.md).
- [Factory-test firmware README](firmware/factory_test/README.md) and [test procedure (Chinese)](firmware/factory_test/docs/test-procedure_CN.md).

## 🤝 Support and Contributions

For questions or improvement suggestions, open a [GitHub Issue](https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-POE-ETH/issues). Include the board revision, connected display or camera, ESP-IDF or Arduino-ESP32 version, reproduction steps, expected and actual behavior, and relevant logs.

## 📄 License

This repository is licensed under the Apache License 2.0. See [LICENSE](LICENSE) for details.
