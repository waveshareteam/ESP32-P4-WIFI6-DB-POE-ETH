# mipi_dsi

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Cycles color bars on the MIPI-DSI panel and tests the I2C backlight controller.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Supported MIPI-DSI panel (5, 7, 8 or 10.1 inch)

## How to Run

1. Install `esp32` (Arduino-ESP32) in the Boards Manager; the version is listed in the repository README.
2. Add the board by following the [board definition guide](../../libraries/board_config/README.md).
3. Set the sketchbook location to `examples/arduino/`, or copy `examples/arduino/libraries/` to the `libraries` folder of your sketchbook.
4. Select `Waveshare ESP32-P4-WIFI6-DB-POE-ETH`.
5. Install `ESP32_Display_Panel` 1.0.4, `ESP32_IO_Expander` 1.1.1 and `esp-lib-utils` 0.2.3, then enable one local LCD driver in `esp_panel_drivers_conf.h`; the default is the JD9365 10.1-inch panel.
6. Open `mipi_dsi.ino`, upload it and open the serial monitor at 115200 baud.

## Expected Result

Horizontal and vertical color bars cycle on the panel.

## Troubleshooting

If `LCD: start complete` appears without an image, check that the driver and timing match the panel. If backlight setup fails, check the device at I2C address `0x45`.
