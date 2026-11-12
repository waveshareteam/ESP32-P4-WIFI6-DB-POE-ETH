# BLE Peripheral GATT

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Runs a NimBLE peripheral on the ESP32-P4 host using the ESP32-C5 Bluetooth controller through ESP-Hosted HCI. The example exposes one custom GATT service with a read/write characteristic and a notification characteristic.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Phone or computer with a BLE scanner

## How to Run

Flash compatible ESP-Hosted Bluetooth firmware to the ESP32-C5, optionally set the advertised device name and notification interval under `BLE Peripheral GATT config`, then build and flash the host application.

```bash
idf.py set-target esp32p4 menuconfig
idf.py build flash monitor
```

## Expected Result

The log prints `advertising as` with the configured device name. A BLE client can read and write the custom value, subscribe to notifications, and receive the counter value periodically.

## Troubleshooting

Confirm that NimBLE is enabled and the ESP32-C5 runs compatible Hosted Bluetooth firmware. The default advertised name is `esp-hosted-nim-gatt`.
