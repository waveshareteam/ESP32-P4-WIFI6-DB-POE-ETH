# MIPI-CSI Camera

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Shows the live MIPI-CSI camera image on the LCD through `esp_video` and the PPA.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- MIPI-DSI panel matching the Board Manager board definition
- OV5647 MIPI-CSI camera

## How to Run

The display and camera settings come from the Board Manager board definition. Select the connected sensor under `Espressif Camera Sensors Configurations`.

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

The backlight turns on and the LCD shows the live camera image; the log prints the video device and frame size.

## Troubleshooting

Run [08_mipi_dsi](../08_mipi_dsi/) first to verify the panel. If `video cam open failed` appears, check the camera FPC orientation and the selected sensor.
