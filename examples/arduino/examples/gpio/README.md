# gpio

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Reads, writes and configures GPIOs interactively from the serial monitor with the Arduino-ESP32 `Console` library.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- USB cable

## How to Run

1. Install `esp32` (Arduino-ESP32) in the Boards Manager; the version is listed in the repository README.
2. Add the board by following the [board definition guide](../../libraries/board_config/README.md).
3. Set the sketchbook location to `examples/arduino/`, or copy `examples/arduino/libraries/` to the `libraries` folder of your sketchbook.
4. Select `Waveshare ESP32-P4-WIFI6-DB-POE-ETH`.
5. Open `gpio.ino`, upload it and open the serial monitor at 115200 baud.

Set the line ending to `Newline`, then enter commands such as `gpio mode 2 out` and `gpio write 2 1`. Enter `help` to list them.

## Expected Result

After `gpio mode 2 out`, the monitor prints `GPIO 2 mode set to out`, and `gpio read 2` prints the current level.

## Troubleshooting

Run `gpio mode` before `gpio write`. Do not use GPIOs reserved by on-board peripherals while those peripherals are active.
