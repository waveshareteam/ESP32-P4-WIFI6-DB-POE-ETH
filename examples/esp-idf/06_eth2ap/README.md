# Ethernet to Wi-Fi AP

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Bridges Ethernet and a Wi-Fi access point at the MAC layer, using the ESP32-C5 as the radio.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Ethernet cable and a router or switch
- Wi-Fi client device

## How to Run

The ESP32-C5 must already run the coprocessor firmware from [03_wifistation/cp](../03_wifistation/cp/).

```bash
idf.py set-target esp32p4 build flash monitor
```

## Expected Result

After the Ethernet link is up, the `eth2ap` access point appears and its clients reach the wired network.

## Troubleshooting

The default SSID and password are `eth2ap` and `12345678`; change them under `Example Configuration`.
