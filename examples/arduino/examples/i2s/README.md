# i2s

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Loops microphone audio back to the speaker through the ES8311 codec.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Speaker

## How to Run

1. Install `esp32` (Arduino-ESP32) in the Boards Manager; the version is listed in the repository README.
2. Add the board by following the [board definition guide](../../libraries/board_config/README.md).
3. Set the sketchbook location to `examples/arduino/`, or copy `examples/arduino/libraries/` to the `libraries` folder of your sketchbook.
4. Select `Waveshare ESP32-P4-WIFI6-DB-POE-ETH`.
5. Open `i2s.ino`, upload it and open the serial monitor at 115200 baud.

## Expected Result

The speaker plays the microphone input, and the serial monitor prints the input peak.

## Troubleshooting

Keep the microphone and speaker apart to reduce feedback.
