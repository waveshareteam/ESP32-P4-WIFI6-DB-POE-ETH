# USB Extended Display

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Makes the board appear to a Windows PC as a USB extended display, with touch and audio.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Supported MIPI-DSI panel (5, 7, 8 or 10.1 inch)
- Windows PC and USB cable for the high-speed USB port
- Speaker

## How to Run

Select the connected panel under `Component config > Board Support Package (ESP32-P4-WIFI6-DB-POE-ETH) > Display > Select LCD type`; the default is the 10.1-inch panel. Install the PC driver as described in [windows_driver](windows_driver/README.md). The first build needs an internet connection.

```bash
idf.py set-target esp32p4 menuconfig
idf.py build flash monitor
```

## Expected Result

After the driver is installed and the USB port is connected, Windows lists a new display and the LCD shows the extended desktop.

## Troubleshooting

If touch controls the wrong display, choose the correct one in Tablet PC Settings. If USB audio stutters, lower `CONFIG_USB_EXTEND_SCREEN_MAX_FPS`. After changing the panel, rebuild and reconnect USB.
