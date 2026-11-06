# MIPI-DSI Display

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Shows a hardware color-bar pattern on the panel. Options in menuconfig switch to a white-screen test, a touch-drawing test or a deep-sleep test.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Supported MIPI-DSI panel (5, 7, 8 or 10.1 inch)

## How to Run

Select the connected panel under `Component config > Board Support Package (ESP32-P4-WIFI6-DB-POE-ETH) > Display > Select LCD type`; the default is the 10.1-inch panel.

```bash
idf.py set-target esp32p4 menuconfig
idf.py build flash monitor
```

## Expected Result

The backlight turns on and vertical color bars appear; the log prints `Show color bar pattern drawn by hardware`.

## Troubleshooting

If the log looks right but the screen stays dark, check the FPC orientation and the panel selection. This board has no LCD reset GPIO.
