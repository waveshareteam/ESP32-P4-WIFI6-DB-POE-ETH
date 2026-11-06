# ESP32-C5 Coprocessor (Bluetooth)

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Controller-only ESP-Hosted firmware for the on-board ESP32-C5, used by [16_bleprph_minimal](../).

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Access to the ESP32-C5 download port

## How to Run

```bash
idf.py set-target esp32c5 build
idf.py -p <C5_PORT> flash
```

## Expected Result

The firmware waits for the ESP32-P4 host over SDIO.

## Troubleshooting

Wi-Fi is disabled in this build; use the firmware from `15_wifi_iperf/cp` for Wi-Fi examples.
