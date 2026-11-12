# 硬件控制台命令使用说明

[English](command-usage.md) | [简体中文](command-usage_ZH.md)

串口参数为 115200 波特率、8-N-1，提示符为 `waveshare>`。每行输入一条命令。输入 `help` 查看命令列表和内置参数说明。方括号表示可选参数，`|` 表示可选项，尖括号中的内容是需要替换的参数，输入时不包含尖括号。

## 内置与系统命令

- `help` — 列出命令。使用 `help <command>` 查看指定命令的内置说明。
- `uname -a` — 显示芯片版本、核心数、ESP-IDF 版本、应用信息、Flash 和 PSRAM 容量及基础 MAC 地址。
- `free` — 显示内部 RAM 和 PSRAM 的总量、空闲量及最大连续块。
- `reboot` — 重启开发板。
- `nvs test` — 将 `factory` NVS 命名空间中的 `boot_count` 数值加一并保存。
- `test all` — 运行内置命令序列。`test report` 显示已保存的报告；`test clear` 清除报告。

## 后台命令

- `watch [--secs <秒数>] <命令> [参数...]` — 先在前台执行一次命令，再在后台重复执行。不指定 `--secs` 时会持续运行，直到停止。
- `jobs` — 列出正在运行的后台命令及其编号。
- `kill <编号>` — 按编号停止一个后台命令。`kill all` 停止所有后台命令。

示例：输入 `watch --secs 30 i2cdetect`，再输入 `jobs` 查看编号，最后输入 `kill 1` 停止（将 `1` 替换为 `jobs` 显示的编号）。

## I2C

共用总线 SDA 为 GPIO7，SCL 为 GPIO8。地址和寄存器值可使用十进制或十六进制。

- `i2cconfig --freq <100000|400000>` — 设置后续 I2C 命令使用的 SCL 频率，单位为 Hz。
- `i2cdetect` — 按当前频率扫描 7 位地址 `0x08` 至 `0x77`；默认频率为 400 kHz。
- `i2cget -c <地址> -r <寄存器> [-l <1..32>]` — 默认读取 1 字节，也可指定读取字节数。
- `i2cset -c <地址> -r <寄存器> <字节> [<字节> ...]` — 从指定寄存器开始写入最多 32 个数据字节。
- `i2cdump -c <地址>` — 读取设备的 `0x00` 至 `0xff` 寄存器。

示例：`i2cconfig --freq 400000`、`i2cget -c 0x18 -r 0x00 -l 4`、`i2cset -c 0x18 -r 0x00 0x12`。

## 存储、USB 与文件

- `mount -t sd [-o freq=20|40] /sdcard` — 挂载 SD 卡，默认频率为 40 MHz。
- `mount -t usb [<挂载路径>]` — 挂载已连接的 USB 存储设备，默认路径为 `/usb`。
- `umount /sdcard` 或 `umount /usb` — 卸载指定存储卷。
- `lsusb` — 启动 USB Host 并列出已连接设备。
- `df` — 显示已挂载存储卷的总容量和可用空间。
- `ls [<路径>]` — 列出目录；默认路径为 `/`，常见路径有 `/sdcard`、`/usb` 和 `/gpio20`。
- `dd if=<源> of=<目标> [bs=<块大小>] [count=<块数>]` — 按块复制数据。`bs` 默认是 `64k`；大小可加 `k` 或 `m`，范围为 512 字节至 4 MiB，且必须为 4 的倍数。
- `cat <路径> [<路径> ...]` — 输出一个或多个文件；每个文件最多输出 64 KiB。
- `echo [<文本...>]` — 输出文本。`echo [<文本...>] > <路径>` 写入文件并覆盖原内容；`echo [<文本...>] >> <路径>` 追加到文件末尾。命令会添加换行符。

`dd` 的输入源可以是 `/dev/urandom`、`/dev/zero` 或文件；输出目标可以是 `/dev/null` 或文件。从 `/dev/urandom` 或 `/dev/zero` 读取时必须指定 `count`；读取文件时不指定 `count` 则持续到文件末尾。`bs` 默认是 65536 字节。

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

## 网络与 Wi-Fi

- `iwconfig` — 显示已配置的 Wi-Fi 名称和当前连接状态。
- `iwconfig wifi0 essid <ssid> [key <密码>]` — 设置 Wi-Fi 名称和可选密码，然后再启动网络接口。
- `ifup wifi0` — 连接 Wi-Fi 接口并获取网络配置。
- `ifdown wifi0` — 断开 Wi-Fi 接口。
- `ifconfig` — 显示网络接口、MAC、IP、子网掩码、网关和 Wi-Fi 状态。

`iperf` 命令格式：

```text
iperf [-s] [-u] [-V] [--help]
      [-c <主机>] [-p <端口>] [-B <地址>] [--cport=<端口>]
      [-l <长度>] [-i <间隔>] [-t <秒数>] [-b <带宽>]
      [-f <格式>] [-S <tos>] [--id=<编号>] [-P <数量>] [--abort]
```

- `-s` 启动服务端；`-c` 启动客户端；`-u` 使用 UDP；`-V` 使用 IPv6；`--help` 显示组件帮助。
- `-p` 设置服务端端口；`-B` 绑定本机地址；`--cport` 设置客户端端口。
- `-l` 设置数据块长度（默认：TCP 16384、IPv4 UDP 1470、IPv6 UDP 1450 字节）；`-i` 设置报告间隔；`-t` 设置运行秒数（默认 10 秒）。
- `-b` 设置目标比特率；小写 `k/m/g` 后缀为十进制单位，大写 `K/M/G` 后缀为二进制单位。
- `-f` 选择输出单位：`b` bit/s、`k` kbit/s、`m` Mbit/s、`K` Kbyte/s 或 `M` Mbyte/s。
- `-S` 设置 IP TOS；`--id` 指定会话编号（默认创建时递增，`all` 表示所有会话）；`-P` 设置并行流数量。
- `--abort` 或 `-a` 停止正在运行的 iPerf 会话。

`ping` 命令格式：

```text
ping [-W <超时>] [-i <间隔>] [-s <大小>] [-c <次数>]
     [-Q <tos>] [<主机>] [--abort]
```

`-W` 设置响应超时；`-i` 设置发包间隔；`-s` 设置数据长度；`-c` 设置请求次数（默认 5 次）；`-Q` 设置 TOS；`<主机>` 可填写域名或 IP 地址；`--abort` 停止正在运行的 ping 会话。

Wi-Fi 命令示例顺序：`iwconfig wifi0 essid <ssid> key <password>`、`ifup wifi0`、`ifconfig`，最后 `ifdown wifi0`。使用 Wi-Fi 命令需要 ESP32-C5 协处理器处于可用状态。

## 显示、触摸与摄像头

- `fbtest [colorbar|red|green|blue|white|black|checker]` — 用指定图案填充屏幕；默认图案为 `colorbar`。
- `backlight <0..100>` — 按百分比设置屏幕背光亮度。
- `evtest` — 在后台持续输出触摸坐标；使用 `kill <编号>` 或 `kill all` 停止。
- `v4l2-ctl --stream` — 在屏幕上启动摄像头预览；`v4l2-ctl --stop` 结束预览。

## 音频

- `amixer` — 显示播放音量和麦克风增益。
- `amixer set Master <0..100>` — 按百分比设置播放音量。
- `amixer set Capture <0..60>` — 按 dB 设置麦克风增益；支持小数。
- `speaker-test [-f <20..20000>]` — 播放正弦音；默认频率为 1000 Hz，使用 `kill <编号>` 停止。
- `arecord [-d <1..60>] <wav路径>` — 录制 16 kHz 单声道 WAV 文件；默认时长为 5 秒。
- `aplay <音频路径>` — 播放 WAV、MP3、AAC、M4A 或 FLAC 文件。
- `alsaloop` — 将麦克风输入转到扬声器，使用 `kill <编号>` 停止。

## GPIO

排针 GPIO 包括 2、3、4、5、20–27、32、33、36、45–48 和 53。GPIO45 控制 SD 卡供电，SD 卡挂载时不可用。GPIO53 控制功放，音频运行时不可用。

- `gpioinfo` — 列出排针 GPIO 及当前限制状态。
- `gpioset <引脚>=<0|1> [<引脚>=<0|1> ...]` — 将一个或多个排针引脚设为低电平（`0`）或高电平（`1`）。
- `gpioset all=<0|1>` — 将所有当前可用的排针引脚设为指定电平；外设占用的引脚会跳过。
- `gpioset --blink all` — 以 1 Hz 切换所有当前可用的排针引脚，使用 `kill <编号>` 停止。
- `gpioget [all|<引脚> [<引脚> ...]]` — 将指定引脚配置为带上拉的输入并读取电平；不带参数时读取所有排针引脚。
- `cat /gpio<引脚>/value` 和 `cat /gpio<引脚>/direction` — 读取引脚电平或方向。
- `echo <0|1> > /gpio<引脚>/value` — 设置引脚输出电平。
- `echo <in|out|low|high> > /gpio<引脚>/direction` — 配置输入/输出方向；`low` 和 `high` 还会设置输出电平。

以上命令列表涵盖控制台内置命令及本示例注册的所有命令。`iperf` 和 `ping` 由 ESP-IDF 组件提供；这里列出的是本控制台使用的命令格式。
