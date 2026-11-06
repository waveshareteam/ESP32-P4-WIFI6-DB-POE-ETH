# Wi-Fi iPerf

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Measures Wi-Fi throughput over the ESP-Hosted SDIO link, with the ESP32-C5 as the radio and iPerf 2.x on a PC as the peer.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Wi-Fi access point on the same LAN as the PC
- PC with iPerf 2.x

## How to Run

Flash the ESP32-C5 coprocessor from [`cp`](cp/), then the ESP32-P4 host from this directory:

```bash
cd cp
idf.py set-target esp32c5 build
idf.py -p <C5_PORT> flash
cd ..
idf.py set-target esp32p4 menuconfig
idf.py build
idf.py -p <P4_PORT> flash monitor
```

Start `iperf.exe -s -p 5001` on the PC, then run this at the `iperf>` prompt:

```text
sta_connect <SSID> <password>
iperf -c <PC_IP> -p 5001 -t 30 -i 1
```

## Expected Result

After `IP_EVENT_STA_GOT_IP4` appears, iperf prints per-second bandwidth and a 30-second summary.

## Troubleshooting

iPerf3 is not compatible. `wifi status` can reset the ESP32-P4 after association, so use the `IP_EVENT_STA_GOT_IP4` log to read the address.
