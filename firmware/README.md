# Firmware

[English](README.md) | [简体中文](README_ZH.md)

The factory image was tested on the ESP32-P4-WIFI6-DB-POE-ETH with a 10.1-inch display. It is a merged image flashed at `0x0`.

| File | Purpose |
| --- | --- |
| `ESP32-P4-WIFI6-DB-POE-ETH_FactoryOnly_20261027.bin` | Restore the factory demo for the 10.1-inch display |
| `ESP32-P4-WIFI6-DB-POE-ETH_ESP32-C5-WIFI-STA_20261027.bin` | ESP32-C5 Wi-Fi coprocessor firmware |

Erasing the flash removes existing data. Replace `PORT` with the board serial port.

```bash
python -m pip install esptool
esptool --chip esp32p4 --port PORT erase-flash
esptool --chip esp32p4 --port PORT write-flash 0x0 ESP32-P4-WIFI6-DB-POE-ETH_FactoryOnly_20261027.bin
```

To flash the ESP32-C5 image, connect to the coprocessor download port and replace `C5_PORT` with that port:

```bash
esptool --chip esp32c5 --port C5_PORT write-flash 0x0 ESP32-P4-WIFI6-DB-POE-ETH_ESP32-C5-WIFI-STA_20261027.bin
```

The [factory firmware source](../examples/factory/) is available in this repository.
