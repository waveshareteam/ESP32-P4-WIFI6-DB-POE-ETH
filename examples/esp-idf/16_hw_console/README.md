# Hardware Console

[English](README.md) | [简体中文](README_ZH.md)

## Overview

A serial command-line example for operating the ESP32-P4-WIFI6-DB-POE-ETH board. It provides commands for system information, I2C, SD and USB storage, Wi-Fi, display and touch, camera, audio, and GPIO. Peripherals are initialized when their commands are used.

## Hardware

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Supported MIPI-DSI panel (5, 7, 8, or 10.1 inch) for display and touch commands
- microSD card, USB storage, OV5647 camera, and speaker for their corresponding commands
- PC with iPerf 2.x for Wi-Fi throughput commands

## Build and Run

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py set-target esp32p4
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth
idf.py build flash monitor
```

Use `help` in the serial console to list available commands. For command syntax and usage examples, see the [command usage guide](docs/command-usage.md).
