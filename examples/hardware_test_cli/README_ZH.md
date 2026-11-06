# 硬件验证固件

[English](README.md) | [简体中文](README_ZH.md)

## 功能

用类 Linux 命令验证开发板硬件的串口控制台，覆盖 I2C、SD 卡、USB 存储、Wi-Fi iperf、显示、触摸、摄像头、音频和排针 GPIO。`main/camera_preview.c` 和 `main/camera_convert.*` 来自 `esp_board_manager` 测试程序，适用本目录的 [`LICENSE`](LICENSE)。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 支持的 MIPI-DSI 屏幕（5、7、8 或 10.1 英寸）
- 相应测试需要的 microSD 卡、USB 存储设备、OV5647 摄像头和扬声器
- 进行 Wi-Fi 测试时需要安装 iPerf 2.x 的电脑

## 怎么运行

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py set-target esp32p4
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth
idf.py build flash monitor
```

输入 `help` 查看全部命令，输入 `test all` 运行自动检查。完整流程见[测试操作手册](docs/test-procedure_ZH.md)。

## 预期现象

出现 `waveshare>` 提示符，`test report` 打印结果表。外设只在命令需要时才初始化。

## 常见问题

进行 Wi-Fi 测试前，先给 ESP32-C5 烧录 `examples/esp-idf/15_wifi_iperf/cp`。固件无法检测 PoE。SD 卡 40 MHz 模式需要高速卡。
