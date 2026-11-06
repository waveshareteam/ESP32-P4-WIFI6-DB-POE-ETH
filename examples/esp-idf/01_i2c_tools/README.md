# I2C Tools

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Provides a serial console with `i2cconfig`, `i2cdetect`, `i2cget`, `i2cset` and `i2cdump` for scanning and reading I2C devices.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- USB cable

## How to Run

```bash
idf.py set-target esp32p4 build flash monitor
```

At the console, configure the on-board bus before using the other commands:

```text
i2cconfig --scl=8 --sda=7
i2cdetect
```

## Expected Result

The `i2c-tools>` prompt appears, and `i2cdetect` prints a table of the addresses that respond.

## Troubleshooting

Run `i2cconfig` first; the other commands depend on it.
