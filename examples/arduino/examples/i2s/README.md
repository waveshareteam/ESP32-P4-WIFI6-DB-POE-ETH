# i2s

[简体中文](README_ZH.md)

## Function

Loop microphone audio back through the ES8311 speaker output.

## Hardware requirements

Development board with microphone and speaker.

## How to run

Open `i2s.ino`, select the Waveshare board, upload it and open the serial monitor at 115200 baud.

## Expected result

The speaker plays the microphone input and the serial monitor prints the input peak.

## FAQ

Keep the microphone and speaker apart to reduce acoustic feedback.
