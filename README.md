# ESP32-P4-WIFI6-DB-POE-ETH

[中文版本](README_CN.md)

ESP-IDF board support package (BSP) and peripheral examples for the
**ESP32-P4-WIFI6-DB-POE-ETH** board: an ESP32-P4 application processor paired
with an ESP32-C5 Wi-Fi 6 / Bluetooth LE coprocessor, RMII Ethernet (PoE-capable),
a selectable MIPI-DSI touch panel, and a MIPI-CSI camera interface.

## ✨ Overview

This repository provides:

- A ready-to-use BSP component (`components/waveshare_bsp/esp32_p4_wifi6_db_poe_eth`)
  exposing `bsp_*` APIs for display, touch, audio, SD card, Ethernet, and the
  expansion header.
- An [ESP Board Manager](https://github.com/espressif/esp-board-manager) board
  definition (`components/waveshare_bmgr/esp32_p4_wifi6_db_poe_eth`) for the same board,
  selected with `idf.py bmgr -b esp32_p4_wifi6_db_poe_eth`.
- A set of ESP-IDF examples (`examples/esp-idf/`) that exercise each on-board
  peripheral individually, from a minimal board check up to LVGL, camera, and
  USB demos.

See the BSP [README](components/waveshare_bsp/esp32_p4_wifi6_db_poe_eth/README.md) /
[README (中文)](components/waveshare_bsp/esp32_p4_wifi6_db_poe_eth/README_zh.md) and
[API reference](components/waveshare_bsp/esp32_p4_wifi6_db_poe_eth/API.md) for the full
pin mapping and API details summarized below, and
[components/README.md](components/README.md) for how the BSP and Board
Manager variants relate to each other.

## 🖥️ Hardware Overview

| Item | Detail |
| --- | --- |
| Main processor | ESP32-P4, dual-core RISC-V HP core + low-power core |
| Flash | 32 MB (QIO) |
| PSRAM | On-chip PSRAM, 200 MHz |
| Wireless | ESP32-C5 (Wi-Fi 6 + Bluetooth LE) over 4-bit SDIO, via `esp_hosted` / `esp_wifi_remote` |
| Ethernet | RMII MAC/PHY (generic IEEE 802.3 PHY driver), PoE-capable |
| Display | Selectable two-lane MIPI-DSI panel: HX8394 (5", 720x1280), ILI9881C (7", 720x1280), or JD9365 (8"/10.1", 800x1280, default) |
| Touch | GT911 capacitive touch controller, polled over shared I2C |
| Audio | ES8311 codec, speaker output + one analog microphone input |
| Storage | microSD over 4-bit SDMMC |
| Camera | MIPI-CSI, 2-lane, via `esp_video` |
| USB | USB Host |
| Expansion | GPIO header, enumerated through `bsp_get_header_gpios()` |
| ESP-IDF target | `esp32p4`, requires ESP-IDF >= 5.5 |

> **Note**
> The BSP does not configure an external RTC or a dedicated LCD reset GPIO; the
> LCD backlight is controlled over the shared I2C bus instead of a GPIO. See
> [components/waveshare_bsp/esp32_p4_wifi6_db_poe_eth/README.md](components/waveshare_bsp/esp32_p4_wifi6_db_poe_eth/README.md)
> for the complete pin assignment.

## 🗂️ Repository Layout

| Path | Purpose |
| --- | --- |
| `examples/esp-idf/` | ESP-IDF peripheral examples for this board |
| `examples/Arduino/` | Arduino-ESP32 examples and the local board definition (`examples/Arduino/esp32/`) |
| `components/waveshare_bsp/esp32_p4_wifi6_db_poe_eth/` | Board support package (BSP) component source, README, and API reference |
| `components/waveshare_bmgr/esp32_p4_wifi6_db_poe_eth/` | ESP Board Manager board definition for the same board |
| `hardware/` | Reserved for hardware documentation (currently empty) |
| `asserts/` | Reserved for images used in documentation (currently empty) |

## 🚀 Getting Started

Install ESP-IDF (>= 5.5), then build the board-check example first:

```bash
cd examples/esp-idf/00_board_check
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

Replace `PORT` with your serial port, for example `COM7` on Windows or
`/dev/ttyACM0` on Linux. Once the board check passes, pick an example from the
table below.

## 🧪 ESP-IDF Examples

| Example | Description |
| --- | --- |
| [00_board_check](examples/esp-idf/00_board_check/) | First example to run: prints chip, flash, and PSRAM info over serial |
| [01_i2c_tools](examples/esp-idf/01_i2c_tools/) | I2C bus scan and diagnostic console tool |
| [02_sdmmc](examples/esp-idf/02_sdmmc/) | microSD card access over 4-bit SDMMC |
| [03_wifistation](examples/esp-idf/03_wifistation/) | Wi-Fi station connection through the ESP32-C5 `esp_hosted` link |
| [04_i2s_es8311](examples/esp-idf/04_i2s_es8311/) | ES8311 audio playback and echo/record over I2S |
| [05_nvs_rw_value](examples/esp-idf/05_nvs_rw_value/) | NVS read/write, persistent boot counter |
| [06_eth2ap](examples/esp-idf/06_eth2ap/) | Ethernet-to-Wi-Fi-AP bridging |
| [07_ethernet_basic](examples/esp-idf/07_ethernet_basic/) | Basic RMII Ethernet bring-up with `esp_netif` |
| [08_mipi_dsi](examples/esp-idf/08_mipi_dsi/) | Simplest MIPI-DSI display bring-up: color bar test pattern |
| [09_lvgl](examples/esp-idf/09_lvgl/) | LVGL benchmark demo on the MIPI-DSI panel |
| [10_mipi_csi](examples/esp-idf/10_mipi_csi/) | MIPI-CSI camera feed displayed on the LCD via `esp_video` and PPA |
| [11_usb_extend_screen](examples/esp-idf/11_usb_extend_screen/) | USB extended-display example |
| [12_generic_gpio](examples/esp-idf/12_generic_gpio/) | LVGL grid showing live level / toggling of expansion-header GPIOs |
| [13_board_manager](examples/esp-idf/13_board_manager/) | Interactive console for bringing up the board through ESP Board Manager and running functional test cases (LCD, touch, audio, camera, SD card, Ethernet, GPIO, ...) |
| [14_ethernet_iperf](examples/esp-idf/14_ethernet_iperf/) | Ethernet TCP/UDP throughput test using an iPerf 2.x PC peer |
| [15_wifi_iperf](examples/esp-idf/15_wifi_iperf/) | ESP32-C5 Wi-Fi TCP/UDP throughput test through the ESP32-P4 ESP-Hosted SDIO link, using an iPerf 2.x PC peer |

## 📡 Ethernet and Wi-Fi

- Ethernet uses the internal EMAC with a generic IEEE 802.3 PHY driver
  (MDC/MDIO/PHY reset on GPIO31/52/51). The BSP only installs the driver; the
  application owns the netif, event loop, and DHCP/static addressing — see
  [Ethernet lifecycle](components/waveshare_bsp/esp32_p4_wifi6_db_poe_eth/README.md#ethernet-lifecycle).
- Wi-Fi and Bluetooth LE are provided by the ESP32-C5 coprocessor over 4-bit
  SDIO (slot 1), through the application's `esp_hosted` / `esp_wifi_remote`
  components, as demonstrated in
  [03_wifistation](examples/esp-idf/03_wifistation/) and
  [06_eth2ap](examples/esp-idf/06_eth2ap/). For Wi-Fi throughput testing, see
  [15_wifi_iperf](examples/esp-idf/15_wifi_iperf/).

## 📄 License

This repository is licensed under the Apache License 2.0. See [LICENSE](LICENSE)
for details.
