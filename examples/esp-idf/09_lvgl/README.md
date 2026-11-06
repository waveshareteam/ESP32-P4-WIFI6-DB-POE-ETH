# LVGL Benchmark

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Runs the LVGL benchmark demo on the MIPI-DSI panel to test display bring-up and rendering performance.

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

The backlight turns on, the demo runs through its scenes, and FPS and render-time statistics appear in the serial log.

## Troubleshooting

This example does not initialize touch; use [12_generic_gpio](../12_generic_gpio/) for touch input.
