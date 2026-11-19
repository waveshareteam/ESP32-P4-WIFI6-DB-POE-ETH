# Firmware

[English](README.md) | [简体中文](README_ZH.md)

The factory images are merged binaries flashed at `0x0`. Choose the file matching your display size.

| File | Purpose |
| --- | --- |
| [ESP32-P4-WIFI6-DB-POE-ETH-10.1-DSI-TOUCH-A_Factory_20261117.bin](ESP32-P4-WIFI6-DB-POE-ETH-10.1-DSI-TOUCH-A_Factory_20261117.bin) | Factory demo firmware for [10.1-DSI-TOUCH-A](https://www.waveshare.com/10.1-DSI-TOUCH-A.htm) |
| [ESP32-P4-WIFI6-DB-POE-ETH-5-DSI-TOUCH-A_Factory_20261117.bin](ESP32-P4-WIFI6-DB-POE-ETH-5-DSI-TOUCH-A_Factory_20261117.bin) | Factory demo firmware for [5-DSI-TOUCH-A](https://www.waveshare.com/5-DSI-TOUCH-A.htm) |
| [ESP32-P4-WIFI6-DB-POE-ETH-8-DSI-TOUCH-A_Factory_20261117.bin](ESP32-P4-WIFI6-DB-POE-ETH-8-DSI-TOUCH-A_Factory_20261117.bin) | Factory demo firmware for [8-DSI-TOUCH-A](https://www.waveshare.com/8-DSI-TOUCH-A.htm) |
| [ESP32-P4-WIFI6-DB-POE-ETH_ESP32-C5-WIFI-STA_20261112.bin](ESP32-C5/ESP32-P4-WIFI6-DB-POE-ETH_ESP32-C5-WIFI-STA_20261112.bin) | ESP32-C5 Wi-Fi coprocessor firmware |

Erasing the flash removes existing data. The example uses the 10.1-DSI-TOUCH-A image. Replace `PORT` with the board serial port and use the file listed for your display.

```bash
python -m pip install esptool
esptool --chip esp32p4 --port PORT erase-flash
esptool --chip esp32p4 --port PORT write-flash 0x0 ESP32-P4-WIFI6-DB-POE-ETH-10.1-DSI-TOUCH-A_Factory_20261117.bin
```

To flash the ESP32-C5 image, connect to the coprocessor download port and replace `C5_PORT` with that port:

```bash
esptool --chip esp32c5 --port C5_PORT write-flash 0x0 ESP32-C5/ESP32-P4-WIFI6-DB-POE-ETH_ESP32-C5-WIFI-STA_20261112.bin
```

The [factory firmware source](../examples/factory/) is available in this repository.
