# Hardware Validation Firmware (factory_test)

[中文版本](README_CN.md)

A single ESP32-P4 firmware for validating every piece of hardware on the
ESP32-P4-WIFI6-DB-POE-ETH board from a serial console, using Linux-style
commands: I2C at 100 k / 400 k, SD card at 20 / 40 MHz, Ethernet and Wi-Fi
iperf throughput, USB mass storage, display / touch / camera / audio, and the
expansion-header GPIOs. Any command can be looped in the background with
`watch` so an oscilloscope can be attached while the bus keeps running.

The step-by-step procedure, probe points and report template are in the
[tester's procedure (Chinese)](docs/test-procedure_CN.md).

## Prerequisites

- ESP-IDF **v6.0.1** plus the Board Manager CLI helper: `pip install esp-bmgr-assist`
- Serial terminal at 115200 8N1 on the USB download port (UART0)
- For Wi-Fi tests the on-board ESP32-C5 must be flashed once with
  `examples/esp-idf/15_wifi_iperf/cp`
- **iperf 2.x** on the PC (the firmware implements the iperf 2 protocol; iperf3 is not compatible)

## Build and flash

From this directory:

```bash
idf.py set-target esp32p4
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth -c ../../components/waveshare_bmgr -a ../../components/waveshare_bmgr/amends/lcd_7_dsi_touch_a
idf.py build
idf.py -p PORT flash monitor
```

- `bmgr` generates `components/gen_bmgr_codes/` (git-ignored). For another panel
  replace the `-a` amend with `lcd_5_dsi_touch_a` / `lcd_8_dsi_touch_a` /
  `lcd_10_1_dsi_touch_a`, re-run `bmgr`, then `build`.
- On boot only the console starts; **no peripheral is initialised** until the
  first command that needs it.

## Command reference

`help` lists every command with a usage line.

| Area | Commands |
|---|---|
| System | `uname -a`, `free`, `reboot`, `nvs test` |
| Background loop | `watch [--secs N] <cmd...>`, `jobs`, `kill <id\|all>` |
| I2C | `i2cconfig --freq 100000\|400000`, `i2cdetect`, `i2cget -c 0x18 -r 0x00`, `i2cset -c 0x18 -r 0x00 0x12`, `i2cdump -c 0x18` |
| SD card | `mount -t sd [-o freq=20\|40] /sdcard`, `umount /sdcard`, `df`, `ls /sdcard` |
| USB stick | `lsusb`, `mount -t usb /usb`, `umount /usb` |
| Throughput / verify | `dd if=/dev/urandom of=/sdcard/t.bin bs=64k count=512`, `dd if=/sdcard/t.bin of=/dev/null bs=64k` |
| Ethernet | `ifup eth0`, `ifdown eth0`, `ifconfig`, `ping <ip>` |
| Wi-Fi | `iwconfig wifi0 essid <ssid> key <pwd>`, `ifup wifi0`, `ifdown wifi0`, `iwconfig` |
| iperf | `iperf -s [-u]`, `iperf -c <ip> [-u] [-t N] [-B <local ip>]`, `iperf -a` |
| Display | `fbtest colorbar\|red\|green\|blue\|white\|black\|checker`, `backlight 0-100`, `evtest` |
| Camera | `v4l2-ctl --stream`, `v4l2-ctl --stop` |
| Audio | `speaker-test [-f 1000]`, `arecord -d 5 /sdcard/rec.wav`, `aplay /sdcard/<file>.wav|.mp3|.aac|.m4a|.flac`, `alsaloop`, `amixer set Master 80`, `amixer set Capture 30` |
| Header GPIO | sysfs-style files: `echo 1 > /gpio20/value`, `cat /gpio20/value`, `echo in > .../direction`, `ls /`; bulk: `gpioinfo`, `gpioset all=1`, `gpioset --blink all`, `gpioget all` |
| Files | `cat <path>`, `echo <text> > <path>`, `echo <text> >> <path>`, `ls <path>` (work on /gpio<N>, /sdcard and /usb) |
| Summary | `test all`, `test report`, `test clear` |

### Background loops (for the oscilloscope)

```
waveshare> watch i2cget -c 0x18 -r 0x00       # returns immediately, repeats forever
[1] started: i2cget -c 0x18 -r 0x00
[1] i2cget -c 0x18 -r 0x00: iter=812 err=0 0.00 MB/s     # one line every 5 s
waveshare> jobs
waveshare> kill 1
[1] done: ...  iter=1630 err=0  10.0 s
```

- `watch` runs the command once in the foreground first; bad arguments or a busy
  resource stop it from starting.
- Several loops may run at once (e.g. SD reads at 40 MHz plus I2C at 400 k to
  look for crosstalk).
- `speaker-test`, `evtest`, `alsaloop`, `gpioset --blink` and `v4l2-ctl --stream`
  are background jobs themselves; stop them with `kill`.

### Result table

Commands with a clear pass criterion record their result; `test report` prints
the table at any time. `test all` runs every automatically judged item (system,
NVS, I2C 100 k / 400 k, SD 20 / 40 MHz write+verify, Ethernet link, USB stick
write+verify) and prints the table. Display, camera, audio and GPIO items are
recorded as MANUAL; iperf figures are copied from the iperf output.

## Known limitations

- PoE cannot be detected by firmware; the procedure asks for a second run on PoE power.
- 40 MHz needs an SD card that supports high-speed mode; `mount` prints the negotiated clock.
- With Ethernet and Wi-Fi both up, the default route is the interface brought up last;
  use `iperf -B <local ip>` to pick one explicitly.
- `gpioset` skips GPIO45 (SD power enable) while the card is mounted and GPIO53 (PA
  enable) while audio is active; writing their sysfs `value` returns `EBUSY`.
- `/gpio<N>` only contains the header pins and needs no `export`; writing `value`
  switches the pin to output automatically. `ls /` lists the GPIOs (/sdcard and
  /usb are separate mount points).
- `i2cdetect` performs a real 1-byte read per address because IDF's
  `i2c_master_probe()` is fixed at 100 kHz.

## License

`main/camera_preview.c` and `main/camera_convert.c/.h` are ported from the
esp-board-manager test_apps (see `examples/esp-idf/13_board_manager`) and keep
their `LicenseRef-Espressif-Modified-MIT` headers; the full text is in
[`LICENSE`](LICENSE). Everything else is Apache-2.0 like the rest of the repository.
