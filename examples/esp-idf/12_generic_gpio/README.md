# GPIO Monitor

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Shows the live level of the expansion-header GPIOs in an LVGL grid and lets you switch them between input and output on the touch screen.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- MIPI-DSI panel matching the Board Manager board definition

## How to Run

The display settings come from the Board Manager board definition. The default GPIO direction and pull mode are under `GPIO Monitor Example`.

For a different panel, replace the `idf.py bmgr` command below with the matching display amendment, for example:

```bash
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth -a ../esp32_p4_dsi_overlay/7_dsi_touch_c
```

Available amendments are `5_dsi_touch_a`, `7_dsi_touch_c`, `8_dsi_touch_a`, `10_1_dsi_touch_a`.

```bash
idf.py set-target esp32p4
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth
idf.py menuconfig
idf.py build flash monitor
```

## Expected Result

The LCD shows one card for each header GPIO with its current level, and the top switch changes the mode of all monitored GPIOs.

## Troubleshooting

Disconnect anything attached to the expansion header before switching to output mode.
