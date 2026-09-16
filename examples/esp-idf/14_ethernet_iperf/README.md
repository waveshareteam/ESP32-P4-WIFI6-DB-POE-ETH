# Ethernet iPerf Example

[中文版](./README_CN.md)

Measure the Ethernet TCP and UDP throughput of the ESP32-P4-WIFI6-DB-POE-ETH board.
Unlike `07_ethernet_basic`, which only verifies Ethernet bring-up and IP connectivity, this example
starts an interactive `iperf>` console after the board receives an IPv4 address through DHCP.

## Board configuration

The example uses the board's internal ESP32-P4 EMAC and IP101 PHY over RMII:

| Signal | GPIO / setting |
| --- | --- |
| PHY | IP101, address 1 |
| MDC / MDIO | GPIO31 / GPIO52 |
| PHY reset | GPIO51 |
| RMII REF_CLK | GPIO50, external 50 MHz input |
| TX_EN / TXD0 / TXD1 | GPIO49 / GPIO34 / GPIO35 |
| CRS_DV / RXD0 / RXD1 | GPIO28 / GPIO29 / GPIO30 |

Connect the board and the test PC to the same Ethernet network. The network must provide DHCP;
the console is intentionally not started until the board has an IPv4 address.

The console command is compatible with iPerf 2.x. It is not compatible with iPerf3.

## Build and run

From this example directory:

```powershell
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

Replace `PORT` with the serial port of the board. After link negotiation and DHCP succeed, the log
contains an `eth0 ip: ...` line and the `iperf>` prompt. Enter the following command to verify the
obtained address:

```text
ethernet info
```

If the prompt never appears, the board has not obtained a DHCP lease. Check the Ethernet link,
switch/router port, and DHCP service before testing throughput.

## Windows PC preparation

Install an iPerf 2.x Windows binary, for example from the
[iPerf2 project downloads](https://sourceforge.net/projects/iperf2/files/). The commands use `iperf`;
adjust the executable name or add its directory to `PATH` if necessary. Replace the example addresses
with your actual PC Ethernet IPv4 address and board IP address.

The PC firewall must allow inbound UDP and TCP port 5001 for the test executable.

## UDP throughput test

### ESP32-P4 to PC

Start the PC server. `-s` means it keeps listening until you stop it with `Ctrl+C`; it does not scan
the network.

```powershell
iperf -u -s -i 3
```

Then run this on the board, replacing `PC_IP` with the PC's wired Ethernet IPv4 address:

```text
iperf -u -c PC_IP -i 3 -t 30
```

### PC to ESP32-P4

Start the board server:

```text
iperf -u -s -t 30 -i 3
```

Then run this on the PC:

```powershell
iperf -u -c ESP_IP -b 80M -t 30 -i 3
```

Start at `80M`, then try `85M`, `90M`, and `95M` only if packet loss remains acceptably low.

## Interpreting results

- A 100 Mbps Ethernet link rate is a PHY-layer rate. The measured UDP payload throughput will be lower
  because of Ethernet, IP, UDP, and software-stack overhead.
- For UDP, record the achieved bandwidth, jitter, and `Lost/Total Datagrams` from the PC output.
  The maximum rate before sustained loss is more meaningful than a single requested rate.
- Lines containing `0.000 Bytes` from an idle PC UDP server mean that it has not received test packets
  during that reporting interval. They do not indicate a network scan or a measured 0 Mbps link.
- Keep the board, PC, cable, and switch port at 100 Mbps full duplex when comparing results.
