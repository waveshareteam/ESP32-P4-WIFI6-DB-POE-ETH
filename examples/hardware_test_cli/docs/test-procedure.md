# ESP32-P4-WIFI6-DB-POE-ETH Hardware Validation Procedure

[English](test-procedure.md) | [简体中文](test-procedure_ZH.md)

Applies to the firmware in [`examples/hardware_test_cli`](../). Use a 115200 8N1
serial terminal; the prompt is `waveshare>`. Enter `help` at any time to list
the available commands.

Conventions:

- Text after `waveshare>` is a command. Other text is representative output;
  values depend on the board and peripherals.
- Copy values in the “Record” fields into Appendix A.
- Use `jobs` to list background tasks and `kill <id>` or `kill all` to stop
  them. Start a background task before connecting an oscilloscope probe, then
  stop it after the measurement.

## 0. Preparation

| Item | Requirement |
| --- | --- |
| Board under test | `hardware_test_cli` flashed; Wi-Fi testing also needs the ESP32-C5 flashed with `examples/esp-idf/15_wifi_iperf/cp` |
| microSD card | FAT32 and High Speed capable; test data may be erased |
| USB flash drive | FAT32, any capacity, working current at or below 500 mA |
| Ethernet cable and switch/router | DHCP service; the PC is on the same network |
| Wi-Fi access point | 2.4 GHz or 5 GHz WPA2; the PC should preferably use Ethernet |
| PC | Serial terminal and iperf 2.x; iperf3 is not compatible |
| Oscilloscope or logic analyzer | At least 100 MHz bandwidth for 40 MHz SD and 12.288 MHz MCLK |
| Multimeter | Header-level measurements |
| Display | Waveshare 7-inch DSI touch display; other sizes require rebuilding with the matching `bmgr` amendment |
| Camera | OV5647 CSI module |
| Speaker | Connected to the onboard amplifier output |
| Optional audio file | An MP3 on the SD card for `aplay` testing |
| PoE equipment | Required for the PoE power test |

After boot, the console should print a banner and wait for input without
initialising peripherals.

## 1. System Information

```text
waveshare> uname -a
chip:      ESP32-P4 rev v1.0, 2 core(s)
idf:       v6.0.1
flash:     32 MB
psram:     32 MB
base mac:  xx:xx:xx:xx:xx:xx
waveshare> free
waveshare> nvs test
nvs ok, boot_count=1
```

Pass criteria: Flash and PSRAM report the expected capacities and NVS reports
`nvs ok`. Record the chip revision and MAC address.

## 2. I2C at 100 kHz and 400 kHz

The shared bus uses SDA GPIO7 and SCL GPIO8. The onboard devices are the ES8311
codec at `0x18`, the backlight controller at `0x45`, and GT911 touch at `0x5d`
or `0x14` when a display is connected.

```text
waveshare> i2cconfig --freq 100000
waveshare> i2cdetect
waveshare> i2cconfig --freq 400000
waveshare> i2cdetect
```

Both scans must pass and the measured SCL frequency must match the selected
setting. To capture a waveform, run:

```text
waveshare> watch i2cget -c 0x18 -r 0x00
```

Probe GPIO8 and GPIO7. Check the SCL period (10 us at 100 kHz and 2.5 us at
400 kHz), rise time, logic levels, and that the job reports `err=0`. Stop it
with `kill <id>`.

## 3. SD Card at 20 MHz and 40 MHz

The SDMMC pins are CLK GPIO43, CMD GPIO44, D0-D3 GPIO39-GPIO42, and power
enable GPIO45.

```text
waveshare> mount -t sd -o freq=20 /sdcard
waveshare> dd if=/dev/urandom of=/sdcard/t.bin bs=64k count=512
waveshare> dd if=/sdcard/t.bin of=/dev/null bs=64k
waveshare> umount /sdcard
waveshare> mount -t sd -o freq=40 /sdcard
waveshare> dd if=/dev/urandom of=/sdcard/t.bin bs=64k count=512
waveshare> dd if=/sdcard/t.bin of=/dev/null bs=64k
```

Both mounts must negotiate the requested clock, and the read-back must verify
without errors. During a transfer, `watch dd if=/sdcard/t.bin of=/dev/null
bs=64k` can be used to measure the bus. Probe GPIO43, GPIO44, and GPIO39, then
unmount the card and stop the job.

## 4. Ethernet

The current hardware test console does not include an Ethernet test. Use the
Ethernet examples for network bring-up and throughput validation.

## 5. Wi-Fi Through the ESP32-C5

The ESP-Hosted SDIO link uses CLK GPIO18, CMD GPIO19, D0-D3 GPIO14-GPIO17, and
C5 reset GPIO54.

```text
waveshare> iwconfig wifi0 essid <SSID> key <password>
waveshare> ifup wifi0
waveshare> ifconfig
waveshare> iperf -c <PC_IP> -t 30
waveshare> iperf -a
waveshare> ifdown wifi0
```

The coprocessor version, association, IP address, and RSSI must be reported.
If the link fails, confirm that the C5 has the compatible coprocessor firmware.
Probe GPIO18 during iperf traffic if the SDIO clock needs to be checked.

## 6. USB Host

```text
waveshare> lsusb
waveshare> mount -t usb /usb
waveshare> dd if=/dev/urandom of=/usb/t.bin bs=64k count=1024
waveshare> dd if=/usb/t.bin of=/dev/null bs=64k
waveshare> umount /usb
```

The device must enumerate and mount, and the read-back must report `verify ok`
with `err=0`. Use `watch dd if=/usb/t.bin of=/dev/null bs=64k` for a sustained
transfer. Probe the USB D+ and D- lines with a suitable differential probe.

## 7. Display, Backlight, and Touch

```text
waveshare> fbtest colorbar
waveshare> fbtest red
waveshare> fbtest green
waveshare> fbtest blue
waveshare> fbtest white
waveshare> fbtest black
waveshare> fbtest checker
waveshare> backlight 10
waveshare> backlight 50
waveshare> backlight 100
waveshare> evtest
```

The colour bars must be ordered correctly, the panel must have no visible dead
pixels or light leakage, all three backlight levels must differ, and touch
coordinates at the corners and centre must follow the finger position. Stop
the touch job with `kill <id>`.

## 8. Camera

```text
waveshare> v4l2-ctl --stream
waveshare> v4l2-ctl --stop
```

The panel should show a stable live OV5647 image with correct colour and
orientation. Record the frame rate from the five-second iteration statistics
and confirm `err=0`.

## 9. Audio

The codec uses MCLK GPIO13, SCLK GPIO12, WS GPIO10, DOUT GPIO9, DIN GPIO11,
and amplifier enable GPIO53.

```text
waveshare> speaker-test
waveshare> amixer set Master 30
waveshare> speaker-test -f 440
waveshare> kill all
waveshare> mount -t sd /sdcard
waveshare> arecord -d 3 /sdcard/rec.wav
waveshare> aplay /sdcard/rec.wav
waveshare> alsaloop
waveshare> amixer set Capture 45
waveshare> kill <id>
```

During playback, check MCLK 12.288 MHz, SCLK 1.536 MHz, and WS 48 kHz. The
speaker output, record/playback loop, and microphone loopback must be clear
without obvious distortion.

## 10. Expansion GPIO

The header test pins are 2, 3, 4, 5, 20, 21, 22, 23, 24, 25, 26, 27, 32, 33,
36, 45, 46, 47, 48, and 53. GPIO45 controls SD power and GPIO53 controls the
amplifier; they are skipped while the corresponding peripheral is active.

```text
waveshare> gpioinfo
waveshare> echo 1 > /gpio20/value
waveshare> echo 0 > /gpio20/value
waveshare> cat /gpio20/value
waveshare> echo in > /gpio20/direction
waveshare> gpioset all=1
waveshare> gpioset all=0
waveshare> gpioset --blink all
waveshare> kill <id>
waveshare> gpioget all
```

Each pin must produce the expected high, low, toggling, and input readings.
Before testing GPIO45, unmount the SD card; before testing GPIO53, stop audio.

## 11. Summary

```text
waveshare> test all
waveshare> test report
```

`test all` runs the automatic system, NVS, I2C, SD-card, and USB checks.
Display, camera, audio, and GPIO results remain manual and must be entered in
the report.

## Appendix A: Test Report

| Item | Result | Value | Notes |
| --- | --- | --- | --- |
| System information | | | |
| NVS read/write | | | |
| I2C at 100 kHz / 400 kHz | | | |
| SD write/read at 20 MHz | | | |
| SD write/read at 40 MHz | | | |
| Wi-Fi connection and throughput | | | |
| USB enumeration and read/write | | | |
| Display, backlight, and touch | | | |
| Camera preview | | | |
| Audio playback and recording | | | |
| Expansion GPIO | | | |

## Appendix B: Probe Points

| Test | Probe point | Expected |
| --- | --- | --- |
| I2C | GPIO8 SCL / GPIO7 SDA | 100 kHz or 400 kHz |
| SD card | GPIO43 CLK / GPIO44 CMD / GPIO39 D0 | 20 MHz or 40 MHz |
| SDIO | GPIO18 CLK / GPIO19 CMD | Active during Wi-Fi traffic |
| RMII | GPIO50 REF_CLK / GPIO49 TX_EN | 50 MHz reference clock |
| USB | Connector D+ / D- | High-Speed or Full-Speed signalling |
| Audio | GPIO13 MCLK / GPIO12 SCLK / GPIO10 WS | 12.288 MHz / 1.536 MHz / 48 kHz |
