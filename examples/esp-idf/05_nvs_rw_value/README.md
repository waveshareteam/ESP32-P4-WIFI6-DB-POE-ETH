# NVS Read and Write

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Stores a boot counter and a string in NVS, so the counter persists across restarts.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- USB cable

## How to Run

```bash
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth
idf.py set-target esp32p4 build flash monitor
```

## Expected Result

Each boot logs `Boot count:` with a higher value, then `Read string: Hello from NVS!`.

## Troubleshooting

Run the following command to reset the counter:

```bash
idf.py erase-flash
```
