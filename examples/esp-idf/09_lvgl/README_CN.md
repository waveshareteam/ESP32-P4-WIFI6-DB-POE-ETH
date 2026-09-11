# LVGL 性能测试

[English Version](./README.md)

通过 BSP 的 LVGL adapter，在开发板的 MIPI-DSI 屏幕上运行 LVGL 内置的
`benchmark` 演示。该示例不使用触摸输入，只用于验证显示 bring-up、LVGL
移植和渲染性能。

## 难度

中级。

## 硬件要求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板。
- 连接到显示接口的 BSP 支持的 MIPI-DSI 屏幕，并在 `menuconfig` 中选择
  匹配的 5、7、8 或 10.1 英寸型号
  （`Component config > Board Support Package > Display > Select LCD type`）。
  该示例当前提交的 `sdkconfig` 选择的是 7 英寸 ILI9881C 面板；如果连接的
  是其他面板，请自行修改。

该示例使用本地 `esp32_p4_wifi6_db_poe_eth` BSP 组件。它调用
`bsp_display_start_with_config()`，旋转角度为 `0`，使用默认的 MIPI-DSI
防撕裂模式，点亮背光后在 LVGL 互斥锁保护下运行 `lv_demo_benchmark()`。

## 构建和烧录

```bash
cd examples/esp-idf/09_lvgl
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

将 `PORT` 替换为你的串口，例如 Windows 上的 `COM7` 或 Linux 上的
`/dev/ttyACM0`。

## 预期行为

串口日志应显示：

```text
Initializing BSP display (800x1280)
LVGL benchmark started
```

（第一行的分辨率取决于所选面板。）LCD 背光点亮后，LVGL 的 benchmark 演示会
依次运行各个测试场景，并在串口打印 FPS / 渲染耗时等统计信息
（`sdkconfig.defaults` 已启用 `CONFIG_LV_USE_SYSMON`、
`CONFIG_LV_USE_PERF_MONITOR` 和 `CONFIG_LV_USE_LOG`）。

## 排障

- 确认显示面板型号和接口，并确认 Kconfig 中的 `BSP_LCD_TYPE_*` 选择与实际
  连接的面板一致。
- 确认 PSRAM 已启用（`CONFIG_SPIRAM=y`）；LVGL 演示和 128 KB 的 LVGL 堆
  （`CONFIG_LV_MEM_SIZE_KILOBYTES`）都依赖 PSRAM。
- 如果日志正常但背光不亮，检查 GPIO7/GPIO8 上的共享 I2C 总线，以及
  `0x45` 设备的 `0x96` 寄存器写入。
- 本板没有 LCD 复位 GPIO；请检查 DSI 供电、lane 配置和 FPC 方向。
- 该示例没有初始化触摸；如果还需要 GT911 触摸输入，请运行
  [12_generic_gpio](../12_generic_gpio/) 或其他调用 `bsp_display_start()`
  的显示示例。
