# ESP32-P4-WIFI6-DB-POE-ETH 硬件验证操作手册

适用固件：`firmware/factory_test`。串口 115200，提示符 `waveshare>`。任何时候输入 `help` 查看命令。

约定：
- `waveshare>` 后面是要敲的命令，其余为预期输出（数值因板而异）。
- "记录"栏抄到附录 A 的报告表。
- 后台任务用 `jobs` 查看、`kill <id>` 或 `kill all` 停止。抓波形时先启动后台任务，再上探头，抓完再 `kill`。

## 0. 测试前准备

| 物料 | 要求 |
|---|---|
| 被测板 | 已烧录 factory_test；Wi-Fi 测试要求 ESP32-C5 已烧 `examples/esp-idf/15_wifi_iperf/cp` |
| microSD 卡 | 支持 High Speed（Class 10 / UHS-I 均可），FAT32 格式，卡上数据可丢弃 |
| U 盘 | FAT32，容量不限，工作电流 ≤ 500 mA |
| 网线 + 交换机/路由器 | 提供 DHCP；PC 在同一网段 |
| Wi-Fi AP | 2.4 G 或 5 G，WPA2；PC 最好通过网线接到同一 AP/路由器 |
| PC | 串口终端；iperf **2.x**（`iperf --version` 确认，不能用 iperf3） |
| 示波器 / 逻辑分析仪 | ≥ 100 MHz 带宽（SD 40 MHz、MCLK 12.288 MHz） |
| 万用表 | 排针电平 |
| 屏幕 | Waveshare 7 寸 DSI 触摸屏（其他尺寸需按 README 重新 `bmgr` 编译） |
| 摄像头 | OV5647 CSI 模组 |
| 喇叭 | 接板载功放输出 |
| 音频文件（可选） | SD 卡上放一个 mp3，用于 `aplay` 试听 |
| PoE 供电设备 | 第 4 节 PoE 项使用 |

上电后应看到：

```
ESP32-P4-WIFI6-DB-POE-ETH factory test console
Type 'help' for the command list. Nothing is initialised until you use it.
waveshare>
```

## 1. 系统信息

```
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

合格：flash 32 MB、psram 显示正确容量、`nvs ok`。记录：芯片版本、MAC。

## 2. I2C（100 kHz / 400 kHz）

总线：SDA=GPIO7，SCL=GPIO8。挂载设备：ES8311 编解码器 `0x18`、背光控制器 `0x45`、GT911 触摸 `0x5d`（或 `0x14`，需接屏）。

```
waveshare> i2cconfig --freq 100000
waveshare> i2cdetect
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:                         -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- 18 -- -- -- -- -- -- --
40: -- -- -- -- -- 45 -- -- -- -- -- -- -- -- -- --
50: -- -- -- -- -- -- -- -- -- -- -- -- -- 5d -- --
...
i2c.detect.100k: PASS (0x18:y 0x45:y gt911:y)
waveshare> i2cconfig --freq 400000
waveshare> i2cdetect
i2c.detect.400k: PASS (...)
```

抓波形：

```
waveshare> watch i2cget -c 0x18 -r 0x00
```

探 GPIO8（SCL）与 GPIO7（SDA）。分别在 100k 和 400k 下测量 SCL 周期（10 µs / 2.5 µs）、上升沿时间、高低电平。抓完 `kill <id>`，确认 `err=0`。

合格：两种频率 `i2cdetect` 均 PASS；波形频率与设置一致，`err=0`。记录：实测 SCL 频率、上升时间。

寄存器读写抽查（可选）：`i2cdump -c 0x18`、`i2cget -c 0x18 -r 0x00`。

## 3. SD 卡（20 MHz / 40 MHz）

引脚：CLK=GPIO43，CMD=GPIO44，D0–D3=GPIO39–42，电源使能=GPIO45。

```
waveshare> mount -t sd -o freq=20 /sdcard
Name: SD32G ... Speed: ...
requested 20 MHz, negotiated 20000 kHz
waveshare> dd if=/dev/urandom of=/sdcard/t.bin bs=64k count=512
33554432 bytes, 2.1 s, 15.2 MB/s
waveshare> dd if=/sdcard/t.bin of=/dev/null bs=64k
33554432 bytes, 1.6 s, 20.0 MB/s  verify ok
waveshare> umount /sdcard
waveshare> mount -t sd -o freq=40 /sdcard
requested 40 MHz, negotiated 40000 kHz
waveshare> dd if=/dev/urandom of=/sdcard/t.bin bs=64k count=512
waveshare> dd if=/sdcard/t.bin of=/dev/null bs=64k
```

抓波形（40 MHz 挂载状态下）：

```
waveshare> watch dd if=/sdcard/t.bin of=/dev/null bs=64k
```

探 GPIO43（CLK）、GPIO44（CMD）、GPIO39（D0）。测 CLK 频率、幅度、过冲。抓完 `kill <id>`，`umount /sdcard`。
20 MHz 同法（先 `umount`，再以 `freq=20` 挂载）。

合格：两种频率均挂载成功且 `negotiated` 等于请求值；读回 `verify ok`；后台循环 `err=0`。
记录：两种频率下写 / 读 MB/s、实测 CLK 频率。

## 4. 以太网

引脚：RMII REF_CLK=GPIO50（PHY 输出 50 MHz），TX_EN=GPIO49，MDC/MDIO=GPIO31/52。

PC 端先起服务端：`iperf -s`（TCP）或 `iperf -s -u`（UDP）。

```
waveshare> ifup eth0
eth0: link up, 100 Mbps full duplex
eth0: ip 192.168.1.50 mask 255.255.255.0 gw 192.168.1.1
waveshare> ifconfig
waveshare> ping 192.168.1.1
waveshare> iperf -c <PC_IP> -t 30            # TCP 上行（板→PC）
waveshare> iperf -c <PC_IP> -u -b 100M -t 30 # UDP 上行
waveshare> iperf -s                          # 板做服务端，PC 端 iperf -c <板IP> -t 30（下行）
waveshare> iperf -a                          # 停止服务端
```

波形：`ifup eth0` 后探 GPIO50 应有 50 MHz 参考时钟；iperf 期间探 GPIO49 TX_EN。

PoE：切换到 PoE 供电（拔掉 USB 供电或按板卡说明），重新上电后重复本节 `ifup eth0` + TCP iperf。

合格：链路 100 Mbps 全双工、拿到 IP、ping 通；TCP 上下行均 ≥ 80 Mbps（参考值，以项目要求为准）；PoE 供电下结果一致。
记录：TCP 上/下行、UDP 上/下行 Mbps 与丢包率；PoE 供电下的 TCP 数值。

## 5. Wi-Fi（ESP32-C5 协处理器）

SDIO 链路：CLK=GPIO18，CMD=GPIO19，D0–D3=GPIO14–17，C5 复位=GPIO54。

```
waveshare> iwconfig wifi0 essid <SSID> key <密码>
waveshare> ifup wifi0
wifi0: starting ESP-Hosted link to ESP32-C5 (SDIO slot 1)...
wifi0: coprocessor fw x.y.z
wifi0: associated
wifi0: ip 192.168.1.51 ...
wifi0: rssi -45 ch 36
waveshare> ifconfig
waveshare> iperf -c <PC_IP> -t 30
waveshare> iperf -s                          # PC: iperf -c <板IP> -t 30
waveshare> iperf -a
waveshare> ifdown wifi0
```

如果 `ifup wifi0` 报 `esp_hosted_init failed` / `connect_to_slave failed`，先确认 C5 已烧固件。
以太网和 Wi-Fi 同时 up 时，用 `iperf -c <PC_IP> -B <wifi0 的 IP>` 指定走 Wi-Fi。

波形：iperf 期间探 GPIO18（SDIO CLK，40 MHz）。

合格：协处理器版本可读、拿到 IP、RSSI 合理（近距离 > −60 dBm）；吞吐以项目要求为准。
记录：协处理器版本、RSSI、信道、TCP 上/下行 Mbps。

## 6. USB Host（U 盘）

```
waveshare> lsusb
usb host started, waiting for devices...
1 device(s) enumerated
MSC device at address 1 (not mounted; use: mount -t usb /usb)
waveshare> mount -t usb /usb
VID:PID 0781:5567  29xxx MB (... sectors x 512 bytes)
mounted at /usb
waveshare> dd if=/dev/urandom of=/usb/t.bin bs=64k count=1024
waveshare> dd if=/usb/t.bin of=/dev/null bs=64k
67108864 bytes, ... MB/s  verify ok
```

抓波形：

```
waveshare> watch dd if=/usb/t.bin of=/dev/null bs=64k
```

探 USB 连接器的 D+ / D−（差分探头最佳）。High-Speed 设备眼图 / 幅度约 ±400 mV，Full-Speed 约 3.3 V。抓完 `kill <id>`，`umount /usb`。

合格：枚举 + 挂载成功，读回 `verify ok`，`err=0`。记录：VID:PID、写 / 读 MB/s、总线速度（HS/FS）。

## 7. 屏幕 / 背光 / 触摸

```
waveshare> fbtest colorbar     # 从左到右：白 黄 青 绿 品红 红 蓝 黑
waveshare> fbtest red
waveshare> fbtest green
waveshare> fbtest blue
waveshare> fbtest white        # 看坏点、亮度均匀性
waveshare> fbtest black        # 看漏光
waveshare> fbtest checker      # 32 px 黑白棋盘，看边缘清晰度
waveshare> backlight 10
waveshare> backlight 50
waveshare> backlight 100
waveshare> evtest              # 用手指划过屏幕四角与中心
touch[0] x=12 y=20
...
waveshare> kill <id>
```

合格：颜色顺序正确、无坏点漏光、背光三档亮度变化明显、触摸五点坐标与位置一致且单调。
记录：屏幕型号、坏点数、触摸是否正常。

## 8. 摄像头

```
waveshare> v4l2-ctl --stream
[3] started: v4l2-ctl
[3] v4l2-ctl: iter=... err=0 ... MB/s     # 每 5 秒帧统计
waveshare> v4l2-ctl --stop
```

合格：屏幕出现实时画面，色彩、方向正常，无撕裂，`err=0`。记录：帧率（5 秒统计里 iter 差 ÷ 5）。

## 9. 音频

引脚：MCLK=GPIO13，SCLK=GPIO12，WS=GPIO10，DOUT=GPIO9（P4→codec），DIN=GPIO11（codec→P4），功放使能=GPIO53。

```
waveshare> speaker-test                  # 1 kHz 正弦波，48 kHz 采样
1000 Hz sine playing; stop with kill <id>. Probe GPIO13 MCLK / GPIO12 SCLK / GPIO10 WS
```

播放期间探：GPIO13 MCLK = 12.288 MHz，GPIO12 SCLK = 1.536 MHz，GPIO10 WS = 48 kHz；喇叭输出用示波器或听感确认 1 kHz 无失真。`kill <id>`。

```
waveshare> amixer set Master 30          # 播放音量 0-100 %，播放中即时生效
waveshare> amixer set Master 90
waveshare> speaker-test -f 440           # 可换频率
waveshare> kill all
waveshare> mount -t sd /sdcard
waveshare> arecord -d 3 /sdcard/rec.wav  # 对着麦克风说话 3 秒
waveshare> aplay /sdcard/rec.wav         # 应回放刚才的声音
waveshare> aplay /sdcard/song.mp3        # 也可放 SD 卡上的 mp3 / aac / m4a / flac（esp_audio_codec 解码）
waveshare> alsaloop                      # 麦克风直通喇叭，说话应即时听到
waveshare> amixer set Capture 45         # 麦克风增益 0-60 dB，直通中即时生效；amixer 查看当前值
waveshare> kill <id>
```

合格：三路时钟频率正确；1 kHz 音清晰无杂音；录放回放清晰；直通无明显延迟。
记录：MCLK/SCLK/WS 实测频率、喇叭输出峰峰值（可选）。

## 10. 扩展排针 GPIO

排针可测引脚（20 个）：2, 3, 4, 5, 20, 21, 22, 23, 24, 25, 26, 27, 32, 33, 36, 45\*, 46, 47, 48, 53\*。
\* GPIO45 是 SD 电源使能，GPIO53 是功放使能；SD 已挂载 / 音频在放时会被自动跳过。测排针前先 `umount /sdcard` 并 `kill all`。

```
waveshare> gpioinfo
waveshare> ls /                        # gpio2 gpio3 ... gpio53
waveshare> echo 1 > /gpio20/value     # 单脚置高，万用表测 3.3 V
waveshare> echo 0 > /gpio20/value     # 单脚置低，0 V
waveshare> cat /gpio20/value          # 读当前电平
waveshare> echo in > /gpio20/direction   # 切回上拉输入：悬空读 1，接 GND 读 0
waveshare> cat /gpio20/direction      # in / out
waveshare> gpioset all=1        # 全部置高，万用表逐脚测 3.3 V
waveshare> gpioset all=0        # 全部置低
waveshare> gpioset --blink all  # 1 Hz 方波，示波器逐脚看
waveshare> kill <id>
waveshare> gpioget all          # 全部设为上拉输入读取
```

合格：每个引脚高 / 低 / 翻转 / 输入读取均正确。记录：异常引脚编号。

## 11. 汇总

```
waveshare> test all             # 自动跑：系统、NVS、I2C 100k/400k、SD 20/40 读写校验、以太网链路、U 盘读写校验
waveshare> test report
ITEM                           RESULT  VALUE                  NOTE
sys.uname                      PASS
nvs.rw                         PASS    count=3
i2c.detect.100k                PASS                           0x18:y 0x45:y gt911:y
i2c.detect.400k                PASS                           0x18:y 0x45:y gt911:y
sd.mount.20M                   PASS    20000 kHz              SD32G
sd.write.20M                   PASS    15.20 MB/s
sd.read.20M                    PASS    20.01 MB/s
sd.mount.40M                   PASS    40000 kHz              SD32G
...
eth.link                       PASS    100 Mbps
usb.msc.mount                  PASS    0781:5567
...
pass=12 fail=0 skip=0 manual=0
```

`test all` 会清空之前的记录；人工项（屏幕/摄像头/音频/GPIO）跑过后会以 MANUAL 出现在表里，需要人工填写结论。

## 附录 A：测试报告表

| 板卡编号 | | 测试人 | | 日期 | |
|---|---|---|---|---|---|

| 项目 | 结果（PASS/FAIL） | 数值 | 备注 |
|---|---|---|---|
| 系统信息（芯片版本 / Flash / PSRAM） | | | |
| NVS 读写 | | | |
| I2C 100 kHz 扫描 | | 实测 SCL: kHz | |
| I2C 400 kHz 扫描 | | 实测 SCL: kHz | |
| SD 20 MHz 写 / 读 | | / MB/s | 实测 CLK: |
| SD 40 MHz 写 / 读 | | / MB/s | 实测 CLK: |
| 以太网链路 / DHCP | | Mbps | |
| 以太网 TCP 上行 / 下行 | | / Mbps | |
| 以太网 UDP 上行 / 下行 | | / Mbps，丢包 % | |
| PoE 供电下以太网 TCP | | Mbps | |
| Wi-Fi 连接（协处理器版本 / RSSI / 信道） | | | |
| Wi-Fi TCP 上行 / 下行 | | / Mbps | |
| USB 枚举 / 挂载 | | VID:PID | HS / FS |
| USB 写 / 读 | | / MB/s | |
| 屏幕（彩条 / 纯色 / 棋盘） | | 坏点数 | |
| 背光三档 | | | |
| 触摸五点 | | | |
| 摄像头预览 | | fps | |
| 音频 1 kHz 播放（MCLK/SCLK/WS） | | / / | |
| 录音 / 回放 | | | |
| 麦克风直通 | | | |
| 排针 GPIO 输出高 / 低 | | 异常引脚: | |
| 排针 GPIO 1 Hz 翻转 | | | |
| 排针 GPIO 输入读取 | | | |

## 附录 B：示波器探测点

| 项目 | 探测点 | 期望 |
|---|---|---|
| I2C | GPIO8 SCL / GPIO7 SDA | 100 kHz 或 400 kHz |
| SD 卡 | GPIO43 CLK / GPIO44 CMD / GPIO39 D0 | 20 MHz 或 40 MHz |
| SDIO（C5） | GPIO18 CLK / GPIO19 CMD | 40 MHz（Wi-Fi 流量时） |
| RMII | GPIO50 REF_CLK / GPIO49 TX_EN | 50 MHz 常在 |
| USB | 连接器 D+ / D− | HS ±400 mV / FS 3.3 V |
| 音频 | GPIO13 MCLK / GPIO12 SCLK / GPIO10 WS | 12.288 MHz / 1.536 MHz / 48 kHz |
| 排针 | 各引脚 | 3.3 V / 0 V / 1 Hz 方波 |
