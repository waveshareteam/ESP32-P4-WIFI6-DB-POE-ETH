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

The on-board I2C bus is initialized automatically with SDA on GPIO7, SCL on GPIO8, and a 100 kHz clock. You can override the pins when needed:

```text
i2cdetect

i2cconfig --scl=8 --sda=7 --freq=100000
i2cdetect
```

## Expected Result

The `i2c-tools>` prompt appears, and `i2cdetect` prints a table of the addresses that respond.

## Troubleshooting

The default bus uses SDA GPIO7 and SCL GPIO8. Run `i2cconfig` only when you need to change the pins or frequency.
