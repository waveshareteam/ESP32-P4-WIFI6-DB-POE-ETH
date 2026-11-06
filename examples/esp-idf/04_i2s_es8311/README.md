# I2S ES8311

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Plays the built-in music clip through the ES8311 codec. Echo mode instead loops the microphone back to the speaker.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Speaker

## How to Run

```bash
idf.py set-target esp32p4 build flash monitor
```

To switch to echo mode, change `Example mode` under `Example Configuration` in menuconfig.

## Expected Result

In music mode the speaker plays `canon.pcm`; in echo mode it plays what the microphone picks up.

## Troubleshooting

In echo mode, keep the microphone and speaker apart to reduce feedback.
