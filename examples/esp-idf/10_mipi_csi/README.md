# MIPI-CSI Camera

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Shows the live MIPI-CSI camera image on the LCD through `esp_video` and the PPA.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Supported MIPI-DSI panel (5, 7, 8 or 10.1 inch)
- OV5647 MIPI-CSI camera

## How to Run

Select the connected panel under `Component config > Board Support Package (ESP32-P4-WIFI6-DB-POE-ETH) > Display > Select LCD type`; the default is the 10.1-inch panel. For another sensor, also change the settings under `Espressif Camera Sensors Configurations`.

```bash
idf.py set-target esp32p4 menuconfig
idf.py build flash monitor
```

## Expected Result

The backlight turns on and the LCD shows the live camera image; the log prints the video device and frame size.

## Troubleshooting

Run [08_mipi_dsi](../08_mipi_dsi/) first to verify the panel. If `video cam open failed` appears, check the camera FPC orientation and the selected sensor.
