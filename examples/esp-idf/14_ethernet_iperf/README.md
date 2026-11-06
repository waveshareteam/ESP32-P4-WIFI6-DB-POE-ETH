# Ethernet iPerf

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Measures Ethernet TCP and UDP throughput from an `iperf>` console, with iPerf 2.x on a PC as the peer.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH development board
- Ethernet cable and a network with DHCP
- PC with iPerf 2.x

## How to Run

```bash
idf.py set-target esp32p4 build flash monitor
```

Start `iperf -u -s -i 3` on the PC, then run this at the `iperf>` prompt:

```text
iperf -u -c PC_IP -i 3 -t 30
```

## Expected Result

The log shows `eth0 ip:` and the `iperf>` prompt, and the PC reports bandwidth, jitter and lost datagrams.

## Troubleshooting

The console starts only after DHCP succeeds. iPerf3 is not compatible, and the PC firewall must allow port 5001.
