# 硬件验证固件（factory_test）

[English](README.md)

给测试人员用的 ESP32-P4-WIFI6-DB-POE-ETH 整板验证固件。一个固件覆盖板上全部硬件，
通过串口控制台用 Linux 风格命令逐项测试：切换 I2C 100k/400k、SD 卡 20/40 MHz、
以太网 / Wi-Fi iperf 吞吐、U 盘读写、屏幕 / 触摸 / 摄像头 / 音频、扩展排针 GPIO；
任何命令都可以用 `watch` 放到后台无限循环，方便示波器抓波形。

逐项操作步骤、示波器探测点和报告表见 [测试操作手册](docs/test-procedure_CN.md)。

## 前置条件

- ESP-IDF **v6.0.1**，并安装 Board Manager 命令行助手：`pip install esp-bmgr-assist`
- 串口终端 115200 8N1，接 USB 下载口（UART0）
- Wi-Fi 测试需要板载 ESP32-C5 已烧录 `examples/esp-idf/15_wifi_iperf/cp`（烧一次即可）
- 吞吐测试 PC 端使用 **iperf 2.x**（固件实现的是 iperf 2 协议，不兼容 iperf3）

## 编译烧录

在本目录执行：

```bash
idf.py set-target esp32p4
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth -c ../../components/waveshare_bmgr -a ../../components/waveshare_bmgr/amends/lcd_7_dsi_touch_a
idf.py build
idf.py -p PORT flash monitor
```

- `bmgr` 会生成 `components/gen_bmgr_codes/`（已 gitignore）。换屏时把 `-a` 换成
  `lcd_5_dsi_touch_a` / `lcd_8_dsi_touch_a` / `lcd_10_1_dsi_touch_a`，重新执行 `bmgr` 再 `build`。
- 固件启动后只起控制台，**不初始化任何外设**；每个外设在第一次用到对应命令时才初始化。

## 命令速查

输入 `help` 可看全部命令和用法。

| 功能 | 命令 |
|---|---|
| 系统 | `uname -a`、`free`、`reboot`、`nvs test` |
| 后台循环 | `watch [--secs N] <命令...>`、`jobs`、`kill <id\|all>` |
| I2C | `i2cconfig --freq 100000\|400000`、`i2cdetect`、`i2cget -c 0x18 -r 0x00`、`i2cset -c 0x18 -r 0x00 0x12`、`i2cdump -c 0x18` |
| SD 卡 | `mount -t sd [-o freq=20\|40] /sdcard`、`umount /sdcard`、`df`、`ls /sdcard` |
| U 盘 | `lsusb`、`mount -t usb /usb`、`umount /usb` |
| 读写测速 | `dd if=/dev/urandom of=/sdcard/t.bin bs=64k count=512`、`dd if=/sdcard/t.bin of=/dev/null bs=64k` |
| 以太网 | `ifup eth0`、`ifdown eth0`、`ifconfig`、`ping <ip>` |
| Wi-Fi | `iwconfig wifi0 essid <ssid> key <pwd>`、`ifup wifi0`、`ifdown wifi0`、`iwconfig` |
| 吞吐 | `iperf -s [-u]`、`iperf -c <ip> [-u] [-t N] [-B <本机IP>]`、`iperf -a` |
| 屏幕 | `fbtest colorbar\|red\|green\|blue\|white\|black\|checker`、`backlight 0-100`、`evtest` |
| 摄像头 | `v4l2-ctl --stream`、`v4l2-ctl --stop` |
| 音频 | `speaker-test [-f 1000]`、`arecord -d 5 /sdcard/rec.wav`、`aplay /sdcard/<file>.wav|.mp3|.aac|.m4a|.flac`、`alsaloop`、`amixer set Master 80`、`amixer set Capture 30` |
| 排针 GPIO | sysfs 虚拟文件：`echo 1 > /gpio20/value`、`cat /gpio20/value`、`echo in > .../direction`、`ls /`；批量：`gpioinfo`、`gpioset all=1`、`gpioset --blink all`、`gpioget all` |
| 文件 | `cat <path>`、`echo <text> > <path>`、`echo <text> >> <path>`、`ls <path>`（对 /gpio<N>、/sdcard、/usb 通用） |
| 汇总 | `test all`、`test report`、`test clear` |

### 后台循环（抓波形）

```
waveshare> watch i2cget -c 0x18 -r 0x00       # 立即返回提示符，后台无限重复
[1] started: i2cget -c 0x18 -r 0x00
[1] i2cget -c 0x18 -r 0x00: iter=812 err=0 0.00 MB/s     # 每 5 秒一行
waveshare> jobs
waveshare> kill 1
[1] done: ...  iter=1630 err=0  10.0 s
```

- `watch` 会先在前台执行一次，参数错误或资源被占用则不启动。
- 可以同时跑多个循环（例如 SD 40 MHz 读 + I2C 400k 读，观察相互干扰）。
- `speaker-test`、`evtest`、`alsaloop`、`gpioset --blink`、`v4l2-ctl --stream` 本身就是后台任务，同样用 `kill` 停。

### 结果汇总

有明确判定的命令会把结果登记到一张表，`test report` 随时打印；`test all` 顺序跑完全部可自动判定的项
（系统、NVS、I2C 100k/400k、SD 20/40 MHz 读写校验、以太网链路、U 盘读写校验）后打印该表。
屏幕 / 摄像头 / 音频 / GPIO 记为 MANUAL，由测试人员目视判断；iperf 数值从输出抄录。

## 已知限制

- PoE 供电固件测不到，手册里作为人工检查项（PoE 供电下重跑一遍）。
- SD 卡 40 MHz 需要卡本身支持高速模式；`mount` 输出里的 `negotiated` 是实际协商到的频率。
- 以太网和 Wi-Fi 同时 up 时默认路由是最后一次 `ifup` 的网口，`iperf -B <本机IP>` 可指定网口。
- GPIO45（SD 电源使能）在 SD 卡挂载期间、GPIO53（功放使能）在音频播放期间会被 `gpioset` 自动跳过，写其 sysfs `value` 返回 `EBUSY`。
- `/gpio<N>` 只包含排针引脚，无需 `export`；写 `value` 会自动把引脚切成输出。`ls /` 列出的是 GPIO（/sdcard、/usb 是独立挂载点，不在其中）。
- `i2cdetect` 对每个地址做真实 1 字节读（IDF 的 `i2c_master_probe` 固定 100 kHz，无法体现 400k）。

## 许可

`main/camera_preview.c`、`main/camera_convert.c/.h` 移植自 Espressif esp-board-manager 的 test_apps
（见 `examples/esp-idf/13_board_manager`），保留其 `LicenseRef-Espressif-Modified-MIT` 头，全文在本目录
[`LICENSE`](LICENSE)。其余文件遵循仓库根目录的 Apache-2.0。
