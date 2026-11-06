# Wi-Fi Station

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Connects to an access point as a Wi-Fi station. The application runs on the ESP32-P4, and the radio runs on the ESP32-C5 over ESP-Hosted SDIO.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Wi-Fi access point

## How to Run

Flash the ESP32-C5 coprocessor from [`cp`](cp/), then the ESP32-P4 host from this directory. Set the SSID and password under `Example Configuration` in menuconfig.

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

The ESP32-P4 log prints `got ip:` with the assigned address, then `connected to ap SSID:`.

## Troubleshooting

The SSID and password default to `myssid` and `mypassword`, so set them before flashing. [`cp_streaming`](cp_streaming/) is an alternative coprocessor build.
