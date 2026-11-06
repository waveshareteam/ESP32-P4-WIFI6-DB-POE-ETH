# Firmware

[English](README.md) | [简体中文](README_ZH.md)

The factory image was tested on the ESP32-P4-WIFI6-DB-POE-ETH with a 10.1-inch display. It is a merged image flashed at `0x0`.

| File | Purpose |
| --- | --- |
| `ESP32-P4-WIFI6-DB-POE-ETH_FactoryOnly_20261027.bin` | Restore the factory demo for the 10.1-inch display |

Erasing the flash removes existing data. Replace `PORT` with the board serial port.

```bash
python -m pip install esptool
esptool --chip esp32p4 --port PORT erase-flash
esptool --chip esp32p4 --port PORT write-flash 0x0 ESP32-P4-WIFI6-DB-POE-ETH_FactoryOnly_20261027.bin
```

The [factory firmware source](../examples/factory/) is available in this repository. The ESP32-C5 coprocessor firmware is distributed separately in the [2026-10-27 release](https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-POE-ETH/releases/tag/2026-10-27).
