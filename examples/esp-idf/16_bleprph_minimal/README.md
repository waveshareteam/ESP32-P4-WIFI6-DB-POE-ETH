# BLE Peripheral (Minimal)

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

A minimal NimBLE peripheral that advertises through the ESP32-C5 Bluetooth controller over ESP-Hosted HCI.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Phone or PC with a BLE scanner

## How to Run

Flash the ESP32-C5 controller from [`cp`](cp/), then the ESP32-P4 host from this directory:

```bash
cd cp
idf.py set-target esp32c5 build
idf.py -p <C5_PORT> flash
cd ..
idf.py set-target esp32p4 menuconfig
idf.py build
idf.py -p <P4_PORT> flash monitor
```

## Expected Result

The log prints `advertising as "esp-hosted-nim"`, and the device can be found and connected from a BLE scanner.

## Troubleshooting

NimBLE supports BLE only, not Classic Bluetooth. Host and coprocessor must use the same transport.
