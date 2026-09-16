# Examples

[简体中文](README_CN.md)

This repository provides both [ESP-IDF](esp-idf/) and [Arduino](Arduino/)
examples for the ESP32-P4-WIFI6-DB-POE-ETH board.

## ESP-IDF examples

ESP-IDF projects live under [esp-idf](esp-idf/). Each directory builds as an
independent ESP-IDF project.

```bash
cd examples/esp-idf/00_board_check
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

The examples are organized around this board's selectable MIPI-DSI display,
GT911 touch controller, ESP32-C5 Wi-Fi 6/BLE coprocessor, PoE-capable RMII
Ethernet, SD card, audio codec, USB, and MIPI-CSI camera.

### Recommended order

1. `00_board_check`: verify toolchain, flashing, serial monitor, flash, and
   PSRAM.
2. `01_i2c_tools`, `05_nvs_rw_value`: learn basic peripheral/runtime patterns
   without display or network hardware.
3. `02_sdmmc`, `04_i2s_es8311`: bring up storage and audio.
4. `08_mipi_dsi`, `09_lvgl`, `12_generic_gpio`: bring up display and touch.
5. `03_wifistation`, `06_eth2ap`, `07_ethernet_basic`: bring up wireless and
   wired networking.
6. `10_mipi_csi`, `11_usb_extend_screen`: use once the matching camera or USB
   hardware path is connected.
7. `13_board_manager`: the ESP Board Manager route (`components/waveshare_bmgr`)
   instead of the BSP; an interactive console that initializes the board and
   runs functional test cases for every declared device.

### ESP-IDF index

| Directory | Purpose | Hardware notes |
| --- | --- | --- |
| [00_board_check](esp-idf/00_board_check/) | First-run board and toolchain check | USB only |
| [01_i2c_tools](esp-idf/01_i2c_tools/) | I2C scanning and tools | Useful for touch/codec/camera bring-up |
| [02_sdmmc](esp-idf/02_sdmmc/) | microSD card access | Requires a microSD card |
| [03_wifistation](esp-idf/03_wifistation/) | Wi-Fi station | Uses the ESP32-C5 ESP-Hosted link |
| [04_i2s_es8311](esp-idf/04_i2s_es8311/) | I2S audio codec | Uses the onboard ES8311 codec |
| [05_nvs_rw_value](esp-idf/05_nvs_rw_value/) | NVS read/write, boot counter | USB only |
| [06_eth2ap](esp-idf/06_eth2ap/) | Ethernet-to-Wi-Fi-AP bridge | Requires an Ethernet cable and the ESP32-C5 Wi-Fi path |
| [07_ethernet_basic](esp-idf/07_ethernet_basic/) | Basic RMII Ethernet bring-up | Requires an Ethernet cable |
| [08_mipi_dsi](esp-idf/08_mipi_dsi/) | MIPI-DSI color bar | Requires a BSP-supported MIPI-DSI panel |
| [09_lvgl](esp-idf/09_lvgl/) | LVGL benchmark | Requires a BSP-supported MIPI-DSI panel |
| [10_mipi_csi](esp-idf/10_mipi_csi/) | MIPI-CSI camera feed on LCD | Requires a camera module and a MIPI-DSI panel |
| [11_usb_extend_screen](esp-idf/11_usb_extend_screen/) | USB extended display | Windows-side driver required |
| [12_generic_gpio](esp-idf/12_generic_gpio/) | Expansion-header GPIO monitor (LVGL) | Requires a MIPI-DSI panel with GT911 touch |
| [13_board_manager](esp-idf/13_board_manager/) | ESP Board Manager bring-up console and test cases | Uses `components/waveshare_bmgr`, needs `esp-bmgr-assist` and `idf.py bmgr` before building; individual cases need the matching panel / card / cable |
| [14_ethernet_iperf](esp-idf/14_ethernet_iperf/) | Ethernet TCP/UDP throughput test | Requires an Ethernet cable, a DHCP network, and a PC running iPerf 2.x |
| [15_wifi_iperf](esp-idf/15_wifi_iperf/) | ESP32-C5 Wi-Fi TCP/UDP throughput test through ESP-Hosted | Requires the onboard ESP32-C5, an AP, two serial ports, and a PC running iPerf 2.x |

## Arduino examples

The [Arduino examples](Arduino/) use Arduino-ESP32 `3.3.11` with the local
`Waveshare ESP32-P4-WIFI6-DB-POE-ETH` board definition under
[Arduino/esp32](Arduino/esp32/). They cover board check, GPIO, I2C, I2S audio,
microSD, and MIPI-DSI/MIPI-CSI display sketches. See the
[Arduino board README](Arduino/esp32/README.md) for board installation and
[each example's own README](Arduino/) for the required board menu settings
and hardware.

Generated outputs (`build/`, `managed_components/`, `dependencies.lock`, and
local `sdkconfig`) are ignored and should not be committed.
