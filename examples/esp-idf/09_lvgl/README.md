# LVGL Benchmark

[中文版本](./README_CN.md)

Run the built-in LVGL `benchmark` demo on the board's MIPI-DSI panel through
the BSP's LVGL adapter. This example does not use touch input; it only
exercises display bring-up, the LVGL port, and rendering performance.

## Difficulty

Intermediate.

## Hardware Required

- ESP32-P4-WIFI6-DB-POE-ETH board.
- One BSP-supported MIPI-DSI panel connected to the display interface.
  Select the matching 5, 7, 8, or 10.1-inch panel in `menuconfig`
  (`Component config > Board Support Package > Display > Select LCD type`).
  The checked-in `sdkconfig` in this example currently selects the 7-inch
  ILI9881C panel; change it if a different panel is connected.

This example uses the local `esp32_p4_wifi6_db_poe_eth` BSP component. It
calls `bsp_display_start_with_config()` with rotation `0` and the default
MIPI-DSI tear-avoidance mode, enables the backlight, then runs
`lv_demo_benchmark()` under the LVGL mutex.

## Build and Flash

```bash
cd examples/esp-idf/09_lvgl
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

Replace `PORT` with your serial port, for example `COM7` on Windows or
`/dev/ttyACM0` on Linux.

## Expected Behavior

The serial log should show:

```text
Initializing BSP display (800x1280)
LVGL benchmark started
```

(The resolution in the first line depends on the selected panel.) The LCD
backlight turns on and LVGL's benchmark demo runs through its scenes,
printing FPS/render-time statistics to the serial log
(`CONFIG_LV_USE_SYSMON`, `CONFIG_LV_USE_PERF_MONITOR`, and
`CONFIG_LV_USE_LOG` are enabled by `sdkconfig.defaults`).

## Troubleshooting

- Confirm the display panel model and interface, and that the Kconfig
  `BSP_LCD_TYPE_*` selection matches the panel actually connected.
- Confirm PSRAM is enabled (`CONFIG_SPIRAM=y`); the LVGL demos and the
  128 KB LVGL heap (`CONFIG_LV_MEM_SIZE_KILOBYTES`) need it.
- If the log looks correct but the backlight stays dark, check the shared
  I2C bus on GPIO7/GPIO8 and the backlight controller at I2C address
  `0x45`, register `0x96`.
- This board has no LCD reset GPIO; verify DSI power, lane configuration,
  and FPC orientation instead.
- This example does not initialize touch; run
  [12_generic_gpio](../12_generic_gpio/) or a display example that calls
  `bsp_display_start()` if you also need GT911 touch input.
