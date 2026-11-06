# Board Check

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Prints the chip, Flash, PSRAM and heap information over serial. Run this example first to confirm that the toolchain, flashing and serial monitor work.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- USB cable

## How to Run

```bash
idf.py set-target esp32p4 build flash monitor
```

## Expected Result

The serial monitor shows the `ESP32-P4 Platform Board Check` banner with the board information, then prints an `alive` log every five seconds.

## Troubleshooting

The Flash size in the log is the size set in this example's `sdkconfig.defaults`, which is smaller than the board's 32 MB Flash.
