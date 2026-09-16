# Wi-Fi iperf

This example measures Wi-Fi throughput on the ESP32-P4-WIFI6-DB-POE-ETH board.
The ESP32-P4 is the ESP-Hosted host, while the onboard ESP32-C5 is the Wi-Fi 6
coprocessor. They communicate over the board's 4-bit SDIO link. The P4 console
associates the C5 with an AP and runs the iperf client or server.

```text
examples/esp-idf/15_wifi_iperf/
├── cp/            ESP32-C5 coprocessor firmware
└── mcu_host/      ESP32-P4 host console application
```

## Prerequisites

- Use an ESP-IDF environment that provides `idf.py`.
- Have access to the serial ports for both the ESP32-C5 and ESP32-P4.
- Connect the PC and the board to the same LAN. For the most representative
  result, connect the PC to the AP through Ethernet rather than the same Wi-Fi
  radio used by the board.
- Use **iperf 2.x** on the PC. The firmware component implements the iperf 2
  protocol and is not compatible with `iperf3`.

## 1. Flash the ESP32-C5 coprocessor

From the repository root:

```bash
cd examples/esp-idf/15_wifi_iperf/cp
idf.py set-target esp32c5
idf.py menuconfig
idf.py -p COM<cp-port> flash monitor
```

The defaults enable the Wi-Fi feature and use SDIO. Keep the coprocessor target
as `esp32c5` and keep its transport configuration consistent with the host.

## 2. Flash the ESP32-P4 host

Open another ESP-IDF terminal and run:

```bash
cd examples/esp-idf/15_wifi_iperf/mcu_host
idf.py set-target esp32p4
idf.py menuconfig
idf.py -p COM<host-port> flash monitor
```

The host defaults select the onboard ESP32-C5 and 4-bit SDIO. They also retain
the ESP-Hosted RPC/Wi-Fi dependencies and throughput-oriented Wi-Fi and LWIP
buffer settings.

## 3. Join an AP from the P4 console

After the P4 starts, the prompt is `iperf>`. `wifi_cmd` is a component name,
not a console command. Use the registered station command instead:

```text
iperf> sta_scan
iperf> sta_connect <SSID> <password>
```

Wait for both an association event and an IPv4 event, for example:

```text
WIFI_EVENT_STA_CONNECTED
IP_EVENT_STA_GOT_IP4:[...][ip=192.168.x.x][mask=...][gw=...]
```

Only start iperf after `IP_EVENT_STA_GOT_IP4` appears. The P4 address shown in
this event is required when the PC runs the iperf client.

Useful commands:

```text
iperf> sta_disconnect
iperf> ping -c 5 <PC_IP>
iperf> iperf --help
```

> [!WARNING]
> With the current dependency set, `wifi status` can reset the P4 after a
> successful association because its station-AID RPC is not packaged. Use the
> `IP_EVENT_STA_GOT_IP4` log above to confirm the host address instead.

## 4. Measure TCP throughput

Run the server on one endpoint and the client on the other. Do not use
`iperf3.exe`; use an iperf 2 executable named `iperf.exe` on the PC.

### P4/C5 to PC

On the PC, start and keep the iperf server running:

```powershell
iperf.exe -s -p 5001
```

On the P4 console, replace `<PC_IP>` with the PC's IPv4 address:

```text
iperf> iperf -c <PC_IP> -p 5001 -t 30 -i 1
```

The console immediately returns to `iperf>` because the traffic task runs in
the background. A successful test prints `Successfully connected`, per-second
bandwidth results, and a 30-second summary.

### PC to P4/C5

On the P4 console, start the server and leave it running:

```text
iperf> iperf -s -p 5001
```

On the PC, replace `<P4_IP>` with the address from `IP_EVENT_STA_GOT_IP4`:

```powershell
iperf.exe -c <P4_IP> -p 5001 -t 30 -i 1
```

Run each direction separately. Compare the final 30-second averages from both
endpoints; small differences between the two reports are normal.

### UDP example

Start a UDP server on the PC:

```powershell
iperf.exe -s -u -p 5001
```

Then send 20 Mbit/s of UDP traffic from the P4:

```text
iperf> iperf -c <PC_IP> -u -b 20 -p 5001 -t 30 -i 1
```

Use `iperf --abort` to stop a running test.
