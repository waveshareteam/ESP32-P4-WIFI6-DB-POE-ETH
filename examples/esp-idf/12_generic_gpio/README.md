# GPIO Monitor

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Shows the live level of the expansion-header GPIOs in an LVGL grid and lets you switch them between input and output on the touch screen.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Supported MIPI-DSI panel (5, 7, 8 or 10.1 inch)

## How to Run

Select the connected panel under `Component config > Board Support Package (ESP32-P4-WIFI6-DB-POE-ETH) > Display > Select LCD type`; the default is the 10.1-inch panel. The default GPIO direction and pull mode are under `GPIO Monitor Example`.

```bash
idf.py set-target esp32p4 menuconfig
idf.py build flash monitor
```

## Expected Result

The LCD shows one card for each header GPIO with its current level, and the top switch changes the mode of all monitored GPIOs.

## Troubleshooting

Disconnect anything attached to the expansion header before switching to output mode.
