# MIPI-DSI Display

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Shows a hardware color-bar pattern on the panel.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- MIPI-DSI panel matching the Board Manager board definition

## How to Run

The panel settings come from the Board Manager board definition. Generate the board code before building.

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

The backlight turns on and vertical color bars appear; the log prints `Show color bar pattern drawn by hardware`.

## Troubleshooting

If the log looks right but the screen stays dark, check the FPC orientation and the board display configuration. This board has no LCD reset GPIO.
