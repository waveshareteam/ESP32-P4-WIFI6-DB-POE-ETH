# Hardware Console Command Usage

[English](command-usage.md) | [简体中文](command-usage_ZH.md)

Connect to the serial console at 115200 baud, 8-N-1. Enter one command per line at `waveshare>`. Run `help` to list commands and their built-in descriptions. Square brackets mark optional arguments, `|` separates alternatives, and angle-bracket values are placeholders to replace without typing the brackets.

## Built-in and system commands

- `help` — list commands. Use `help <command>` to show one command's built-in description.
- `uname -a` — display chip revision, core count, ESP-IDF version, app information, flash and PSRAM sizes, and base MAC address.
- `free` — display total, free, and largest-block heap sizes for internal RAM and PSRAM.
- `reboot` — restart the board.
- `nvs test` — increment and store the `boot_count` value in the `factory` NVS namespace.
- `test all` — run the built-in command sequence. `test report` displays the stored report; `test clear` clears it.

## Background commands

- `watch [--secs <seconds>] <command> [args...]` — run a command once in the foreground, then repeat it in the background. Omit `--secs` to keep repeating until stopped.
- `jobs` — list active background commands and their IDs.
- `kill <id>` — stop one background command. `kill all` stops all background commands.

Example: `watch --secs 30 i2cdetect`, then `jobs`, then `kill 1` (replace `1` with the ID shown by `jobs`).

## I2C

The shared bus uses SDA GPIO7 and SCL GPIO8. Addresses and register values can be entered in decimal or hexadecimal.

- `i2cconfig --freq <100000|400000>` — set the SCL frequency, in Hz, for subsequent I2C commands.
- `i2cdetect` — scan 7-bit addresses `0x08` through `0x77` at the configured frequency; the default is 400 kHz.
- `i2cget -c <address> -r <register> [-l <1..32>]` — read 1 byte by default, or the specified number of bytes.
- `i2cset -c <address> -r <register> <byte> [<byte> ...]` — write up to 32 data bytes starting at the selected register.
- `i2cdump -c <address>` — read registers `0x00` through `0xff` from the selected device.

Examples: `i2cconfig --freq 400000`, `i2cget -c 0x18 -r 0x00 -l 4`, and `i2cset -c 0x18 -r 0x00 0x12`.

## Storage, USB, and files

- `mount -t sd [-o freq=20|40] /sdcard` — mount an SD card. The default clock is 40 MHz.
- `mount -t usb [<mount-path>]` — mount a connected USB storage volume. The default path is `/usb`.
- `umount /sdcard` or `umount /usb` — unmount the selected volume.
- `lsusb` — start the USB host and list connected devices.
- `df` — show total and available space for mounted volumes.
- `ls [<path>]` — list a directory. The default path is `/`; common paths are `/sdcard`, `/usb`, and `/gpio20`.
- `dd if=<source> of=<destination> [bs=<size>] [count=<blocks>]` — copy data in blocks. `bs` defaults to `64k`; sizes may use `k` or `m`, and must be 512 bytes through 4 MiB and divisible by 4.
- `cat <path> [<path> ...]` — print one or more files; output is limited to 64 KiB per file.
- `echo [<text...>]` — print text. `echo [<text...>] > <path>` writes to a file, replacing its contents; `echo [<text...>] >> <path>` appends. A newline is added.

`dd` sources can be `/dev/urandom`, `/dev/zero`, or a file. Destinations can be `/dev/null` or a file. Reading from `/dev/urandom` or `/dev/zero` requires `count`; reading a file without `count` continues to its end. `bs` defaults to 65536 bytes.

```text
mount -t sd -o freq=20 /sdcard
mount -t usb /usb
ls /sdcard
dd if=/dev/urandom of=/sdcard/data.bin bs=64k count=16
dd if=/sdcard/data.bin of=/dev/null bs=64k
cat /sdcard/readme.txt
echo hello >> /sdcard/log.txt
umount /sdcard
```

## Network and Wi-Fi

- `iwconfig` — show configured Wi-Fi name and current connection status.
- `iwconfig wifi0 essid <ssid> [key <password>]` — set the Wi-Fi name and optional password before bringing the interface up.
- `ifup wifi0` — connect the Wi-Fi interface and obtain network settings.
- `ifdown wifi0` — disconnect the Wi-Fi interface.
- `ifconfig` — show network interface, MAC, IP, netmask, gateway, and Wi-Fi status.

`iperf` supports the following syntax:

```text
iperf [-s] [-u] [-V] [--help]
      [-c <host>] [-p <port>] [-B <address>] [--cport=<port>]
      [-l <length>] [-i <interval>] [-t <seconds>] [-b <bandwidth>]
      [-f <format>] [-S <tos>] [--id=<id>] [-P <count>] [--abort]
```

- `-s` starts a server; `-c` starts a client; `-u` selects UDP; `-V` uses IPv6; `--help` prints component help.
- `-p` sets the server port; `-B` binds a local address; `--cport` sets the client port.
- `-l` sets packet length (defaults: TCP 16384, IPv4 UDP 1470, IPv6 UDP 1450 bytes); `-i` sets report interval; `-t` sets duration (default 10 seconds).
- `-b` sets target bit rate. Lowercase `k/m/g` suffixes are decimal; uppercase `K/M/G` suffixes are binary.
- `-f` selects output units: `b` bits/s, `k` kbits/s, `m` Mbits/s, `K` Kbytes/s, or `M` Mbytes/s.
- `-S` sets IP TOS; `--id` identifies an instance (defaults to an incrementing ID; `all` targets all sessions); `-P` sets parallel streams.
- `--abort` or `-a` stops active iPerf sessions.

`ping` supports the following syntax:

```text
ping [-W <timeout>] [-i <interval>] [-s <size>] [-c <count>]
     [-Q <tos>] [<host>] [--abort]
```

`-W` sets response timeout; `-i` sets interval between packets; `-s` sets payload size; `-c` sets request count (default 5); `-Q` sets TOS; `<host>` is a domain name or IP address; `--abort` stops active ping sessions.

Example Wi-Fi sequence: `iwconfig wifi0 essid <ssid> key <password>`, `ifup wifi0`, `ifconfig`, then `ifdown wifi0`. The ESP32-C5 coprocessor must be available for Wi-Fi commands.

## Display, touch, and camera

- `fbtest [colorbar|red|green|blue|white|black|checker]` — fill the display with the selected pattern. The default is `colorbar`.
- `backlight <0..100>` — set display backlight level as a percentage.
- `evtest` — start a background stream of touch coordinates; stop it with `kill <id>` or `kill all`.
- `v4l2-ctl --stream` — start camera preview on the display. `v4l2-ctl --stop` ends the preview.

## Audio

- `amixer` — display playback volume and microphone gain.
- `amixer set Master <0..100>` — set playback volume in percent.
- `amixer set Capture <0..60>` — set microphone gain in dB; decimal values are accepted.
- `speaker-test [-f <20..20000>]` — play a sine tone. The default frequency is 1000 Hz; stop it with `kill <id>`.
- `arecord [-d <1..60>] <wav-path>` — record 16 kHz mono audio to a WAV file. Duration defaults to 5 seconds.
- `aplay <audio-path>` — play a WAV, MP3, AAC, M4A, or FLAC file.
- `alsaloop` — route microphone input to the speaker until stopped with `kill <id>`.

## GPIO

The expansion header GPIOs are 2, 3, 4, 5, 20–27, 32, 33, 36, 45–48, and 53. GPIO45 controls SD power and is unavailable while the SD card is mounted. GPIO53 controls the amplifier and is unavailable while audio is active.

- `gpioinfo` — list expansion header GPIOs and current restrictions.
- `gpioset <pin>=<0|1> [<pin>=<0|1> ...]` — set one or more header pins low (`0`) or high (`1`).
- `gpioset all=<0|1>` — set all available header pins to the selected level; pins reserved by active peripherals are skipped.
- `gpioset --blink all` — toggle all currently available header pins at 1 Hz until stopped with `kill <id>`.
- `gpioget [all|<pin> [<pin> ...]]` — configure selected pins as inputs with pull-ups and read their levels. With no argument, all header pins are read.
- `cat /gpio<pin>/value` and `cat /gpio<pin>/direction` — read a pin's value or direction.
- `echo <0|1> > /gpio<pin>/value` — set the pin's output level.
- `echo <in|out|low|high> > /gpio<pin>/direction` — configure input/output direction. `low` and `high` also set the output level.

The command lists above cover the built-in console command and every command registered by this example. `iperf` and `ping` are provided by their ESP-IDF components; the forms shown here are the forms used by this console.
