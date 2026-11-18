# Wi-Fi Station

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Connects the ESP32-P4 host to a Wi-Fi access point through the on-board ESP32-C5 and ESP-Hosted SDIO transport.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Wi-Fi access point

## How to Run

Flash compatible ESP-Hosted firmware to the ESP32-C5, then set the SSID and password under `Example Configuration` and build the host application.

```bash
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth
idf.py set-target esp32p4 menuconfig
idf.py build flash monitor
```

## Expected Result

The log reports `got ip:` followed by `connected to ap SSID:` after the station joins the configured access point.

## Troubleshooting

The default SSID and password are `myssid` and `mypassword`; update them before flashing and ensure the ESP32-C5 firmware is compatible with the host ESP-Hosted configuration.
