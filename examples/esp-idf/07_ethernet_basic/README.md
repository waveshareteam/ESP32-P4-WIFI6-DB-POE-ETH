# Basic Ethernet

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Brings up the RMII Ethernet port with `esp_netif` and requests an address over DHCP.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Ethernet cable and a network with DHCP

## How to Run

```bash
idf.py set-target esp32p4 build flash monitor
```

## Expected Result

The log shows `Ethernet Link Up`, then `Ethernet Got IP Address` with the address, which you can ping from a PC.

## Troubleshooting

If no address appears, check the cable and the DHCP service.
