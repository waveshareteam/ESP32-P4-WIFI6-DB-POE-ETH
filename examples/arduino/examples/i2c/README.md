# i2c

[简体中文](README_ZH.md)

## Function

Scan the I2C1 bus and print responding 7-bit addresses.

## Hardware requirements

Development board and any I2C peripherals to scan.

## How to run

Open `i2c.ino`, select the Waveshare board, upload it and open the serial monitor at 115200 baud.

## Expected result

Detected addresses appear; scanning repeats every five seconds.

## FAQ

An address is listed only when the device acknowledges.
