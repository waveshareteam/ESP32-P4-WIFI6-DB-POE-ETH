# mipi_csi

[简体中文](README_ZH.md)

## Function

Display the OV5647 camera stream on a MIPI-DSI panel.

## Hardware requirements

Development board, OV5647 camera and compatible DSI panel.

## How to run

Configure the connected panel in `lcd_panel.h`, upload `mipi_csi.ino`, and open the serial monitor at 115200 baud.

## Expected result

The camera image appears on the panel.

## FAQ

The Arduino build must enable the MIPI-CSI video device; see the Chinese guide for configuration details.
