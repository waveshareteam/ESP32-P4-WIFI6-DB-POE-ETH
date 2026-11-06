# Hardware Validation Firmware

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

A serial console that validates the board hardware with Linux-style commands, covering I2C, SD card, USB storage, Wi-Fi iperf, display, touch, camera, audio and header GPIOs. `main/camera_preview.c` and `main/camera_convert.*` come from the `esp_board_manager` test application and are covered by the [`LICENSE`](LICENSE) in this directory.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Supported MIPI-DSI panel (5, 7, 8 or 10.1 inch)
- microSD card, USB storage, OV5647 camera and speaker for the matching tests
- PC with iPerf 2.x for Wi-Fi tests

## How to Run

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py set-target esp32p4
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth
idf.py build flash monitor
```

Type `help` for every command, and `test all` to run the automatic checks. The full procedure is in the [test procedure (Chinese)](docs/test-procedure_ZH.md).

## Expected Result

The `waveshare>` prompt appears, and `test report` prints the result table. Peripherals are initialized only when a command needs them.

## Troubleshooting

For Wi-Fi tests, flash the ESP32-C5 with `examples/esp-idf/15_wifi_iperf/cp` first. PoE cannot be detected by firmware. SD at 40 MHz needs a high-speed card.
