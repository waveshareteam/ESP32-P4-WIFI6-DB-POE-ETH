# eth_ip101

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Uses the onboard IP101 PHY to demonstrate Ethernet events and send an HTTP request after obtaining an IP address.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- USB data cable and Ethernet cable
- Router or network with DHCP enabled

## How to Run

1. Install the Arduino-ESP32 version listed in the [repository README](../../../../README.md) and follow the [board definition guide](../../libraries/board_config/README.md).
2. Connect the Ethernet cable, select `Waveshare ESP32-P4-WIFI6-DB-POE-ETH` in Arduino IDE, and upload `eth_ip101.ino`.
3. Open the serial monitor at 115200 baud.

## Expected Result

The serial monitor prints `ETH Connected` when the link comes up and `ETH Got IP` after DHCP assigns an address. The sketch then sends an HTTP request to `google.com:80` and prints the response it receives.

## Troubleshooting

If `ETH Connected` appears without `ETH Got IP`, check the network's DHCP settings. If the sketch obtains an IP address but prints `connection failed`, check DNS and Internet access.
