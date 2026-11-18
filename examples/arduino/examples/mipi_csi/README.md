# mipi_csi

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Shows the OV5647 camera stream on a MIPI-DSI panel.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Supported MIPI-DSI panel (5, 7, 8 or 10.1 inch)
- OV5647 MIPI-CSI camera

## How to Run

1. Install `esp32` (Arduino-ESP32) in the Boards Manager; the version is listed in the repository README.
2. Add the board by following the [board definition guide](../../libraries/board_config/README.md).
3. Set the sketchbook location to `examples/arduino/`, or copy `examples/arduino/libraries/` to the `libraries` folder of your sketchbook.
4. Select `Waveshare ESP32-P4-WIFI6-DB-POE-ETH`.
5. Select the connected panel with `LCD_PANEL_TYPE` in `lcd_panel.h`; the default is the JD9365 10.1-inch panel.
6. Open `mipi_csi.ino`, upload it and open the serial monitor at 115200 baud.

## Expected Result

The camera image appears on the panel.

## Troubleshooting

`ESP_Video` must be built with `CONFIG_ESP_VIDEO_ENABLE_MIPI_CSI_VIDEO_DEVICE=y`; keep this setting in a custom Arduino-ESP32 build.
