# SD Card (SDMMC)

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Mounts a microSD card over 4-bit SDMMC, prints the card information, then writes, renames and reads back a test file.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- microSD card

## How to Run

```bash
idf.py set-target esp32p4 build flash monitor
```

## Expected Result

The serial monitor shows the card name, type, capacity and speed, followed by the line read back from the test file.

## Troubleshooting

Back up the card first: enabling the format options in `SD/MMC Example Configuration` erases its data.
