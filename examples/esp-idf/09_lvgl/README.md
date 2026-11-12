# LVGL Benchmark

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Runs the LVGL benchmark demo on the MIPI-DSI panel to test display bring-up and rendering performance.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- MIPI-DSI panel matching the Board Manager board definition

## How to Run

The panel and touch settings come from the Board Manager board definition. Generate the board code before building.

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

The backlight turns on, the demo runs through its scenes, and FPS and render-time statistics appear in the serial log.

## Troubleshooting

The example initializes the board-managed touch device.
