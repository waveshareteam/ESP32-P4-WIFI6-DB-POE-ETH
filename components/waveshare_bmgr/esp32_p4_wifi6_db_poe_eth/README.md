# ESP32-P4-WIFI6-DB-POE-ETH Board Definition

[中文](README_zh.md)

This directory provides the ESP Board Manager definition for the Waveshare
`ESP32-P4-WIFI6-DB-POE-ETH`. The board uses the `esp32p4` chip and
configuration version `1.0.0`. It covers the same LCD, touch, audio, camera,
SD card, and RMII Ethernet hardware as the
[`waveshare_bsp` variant](../../waveshare_bsp/esp32_p4_wifi6_db_poe_eth/README.md)
of this board.

## Default configuration

- LCD: JD9365 MIPI-DSI, 800x1280, RGB565, 2 lanes, 1500 Mbps;
- Touch: GT911, 800x1280 coordinate range, with `0xBA` and `0x28` as candidate
  I2C addresses;
- Audio: ES8311 at I2C address `0x30`, I2S port 0, 48 kHz sample rate;
- Camera: CSI camera resource, with OV5647 defaults provided by
  `sdkconfig.defaults.board`;
- Storage: 4-bit SDMMC mounted at `/sdcard`, using `SDMMC_FREQ_HIGHSPEED`;
- MIPI power: LDO channel 3 at 2500 mV;
- LCD uses 3 DPI frame buffers by default, with amends disabled;
- Ethernet: RMII MAC + IP101 PHY (`espressif/ip101` driver), PHY address
  `1`, `CONFIG_ETH_USE_ESP32_EMAC` enabled by default.

## Peripheral mapping

| Peripheral | Configuration |
| --- | --- |
| I2C | Port 1, SDA GPIO7, SCL GPIO8 |
| I2S | Port 0, MCLK/BCLK/WS/DOUT/DIN on GPIO13/12/10/9/11 |
| Amplifier control | GPIO53 |
| SDMMC | CLK/CMD/D0-D3 on GPIO43/44/39/40/41/42 |
| SD power | GPIO45 |
| MIPI-DSI | Bus 0, 2 lanes, 1500 Mbps |
| Ethernet MDC/MDIO | GPIO31 / GPIO52 |
| Ethernet PHY reset | GPIO51 |
| RMII 50 MHz clock in | GPIO50 (`EMAC_CLK_EXT_IN`, supplied by the PHY) |
| RMII TX_EN/TXD0/TXD1 | GPIO49 / GPIO34 / GPIO35 |
| RMII CRS_DV/RXD0/RXD1 | GPIO28 / GPIO29 / GPIO30 |

Not modeled by Board Manager (no corresponding device/peripheral type):
the ESP32-C5 Hosted Wi-Fi SDIO wiring (CLK GPIO18, CMD GPIO19, D0-D3
GPIO14-17, reset GPIO54, slot 1). See the comment block at the end of
`board_peripherals.yaml`. An application configures this transport directly.

## Files

- `board_info.yaml`: board metadata;
- `board_devices.yaml`: audio, LCD, touch, camera, SD card, and Ethernet
  devices;
- `board_peripherals.yaml`: bus, GPIO, LDO, and MIPI-DSI configuration;
- `setup_device.c`: LCD and GT911 touch factory hooks;
- `ethernet.c`: `custom` device init/deinit for the RMII Ethernet MAC/PHY;
- `sdkconfig.defaults.board`: default Flash, PSRAM, LCD, OV5647, and
  Ethernet settings.

## Ethernet usage

`esp-board-manager` has no built-in `ethernet` device type, so this board
declares Ethernet as a `type: custom` device named `ethernet`, following the
custom-device pattern described in the
[esp-board-manager documentation](https://github.com/espressif/esp-board-manager). After `esp_board_manager_init()`, retrieve the installed
driver handle and finish bring-up (event loop, netif/glue, `esp_eth_start()`)
in the application:

```c
#include "esp_eth.h"
#include "esp_board_manager.h"

esp_eth_handle_t eth_handle = NULL;
ESP_ERROR_CHECK(esp_board_manager_get_device_handle("ethernet", (void **)&eth_handle));
/* attach a netif/glue, then: */
ESP_ERROR_CHECK(esp_eth_start(eth_handle));
```

Ethernet MAC/PHY initialization requires `CONFIG_ETH_USE_ESP32_EMAC=y`
(enabled by default in `sdkconfig.defaults.board`); when it is disabled the
device init returns without a handle.

## Usage

Run from the ESP-IDF project directory, pointing `-c` at the
`components/waveshare_bmgr` directory of this repository (the board package is
not on the component registry, so it is not discovered automatically):

```bash
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth -c <repo>/components/waveshare_bmgr
```

To use another Waveshare DSI touch display, add `-a` with the corresponding LCD
amend under `amends/`. See [`../amends/README.md`](../amends/README.md) for the
available display profiles, and
[`examples/esp-idf/13_board_manager`](../../../examples/esp-idf/13_board_manager/README.md)
for a complete project.

LCD, touch, camera, and Ethernet parameters should be checked against the
actual hardware revision. This README describes the Board Manager
configuration in this repository and does not replace hardware validation.
