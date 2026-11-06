# Board Manager Test Application

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

An interactive console that starts the board through ESP Board Manager and runs LCD, touch, audio, camera, SD card and GPIO test cases. It is adapted from the `esp_board_manager` test application, and its Espressif sources are covered by the [`LICENSE`](LICENSE) in this directory.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Supported MIPI-DSI panel (5, 7, 8 or 10.1 inch)

## How to Run

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py set-target esp32p4
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth
idf.py build flash monitor
```

The default display is the 10.1-inch DSI panel. For another panel, add the matching amend directory (`5_dsi_touch_a`, `7_dsi_touch_c`, `8_dsi_touch_a`, `10_1_dsi_touch_a` or `10_1_dsi_touch_b`):

```bash
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth -a ../esp32_p4_dsi_overlay/7_dsi_touch_c
```

## Expected Result

A console prompt appears. Run `bmgr init`, `case list`, then a case such as `case run lcd.pattern.checker`.

## Troubleshooting

`idf.py build` needs `bmgr` to have run at least once; run it again after changing the panel. The console uses UART0, the USB download port.
