# sdmmc

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Reads and writes a test file on a microSD card over SDMMC.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- microSD card

## How to Run

1. Install `esp32` (Arduino-ESP32) in the Boards Manager; the version is listed in the repository README.
2. Add the board by following the [board definition guide](../../libraries/board_config/README.md).
3. Set the sketchbook location to `examples/arduino/`, or copy `examples/arduino/libraries/` to the `libraries` folder of your sketchbook.
4. Select `Waveshare ESP32-P4-WIFI6-DB-POE-ETH`.
5. Open `sdmmc.ino`, upload it and open the serial monitor at 115200 baud.

## Expected Result

The card information and the content read back from the test file appear in the serial monitor.

## Troubleshooting

The example writes `/arduino_test.txt` to the card root.
