## USB Extended Display Example

[简体中文](README_ZH.md)

Use [LaunchPad](https://espressif.github.io/esp-launchpad/?flashConfigURL=https://dl.espressif.com/AE/esp-iot-solution/config.toml) to flash this example.

The USB extended display example turns the ESP32-P4-WIFI6-DB-POE-ETH into a secondary display for Windows. Board peripherals are described and initialized by [ESP Board Manager](https://github.com/espressif/esp-board-manager).

The example accesses the board peripherals by their Board Manager device names. It requires a landscape `display_lcd` device with an RGB565 or RGB888 framebuffer. Touch uses `lcd_touch`; USB audio uses `audio_dac` and `audio_adc`. The default Waveshare board definition configures an 800×1280 portrait panel, which this example does not support. Select a supported landscape display amendment with `-a` before building. Backlight control is available for LEDC-backed `lcd_brightness` devices; custom brightness devices are not handled. The 10.1B display is not supported.

The example supports the following features:

* Supports landscape RGB565 or RGB888 panels configured by Board Manager.
* Supports up to **five-point touch input**.
* Supports audio input and output through the board's audio codec.

## Required Hardware

1. ESP32-P4-WIFI6-DB-POE-ETH development board.
2. A supported landscape MIPI-DSI display and matching Board Manager display amendment. The default 800×1280 portrait display configuration and 10.1B display are not supported by this example.
3. A speaker, if USB audio is enabled.

## Hardware Connection

1. Connect the high-speed USB port on the development board to the PC.

## Compilation and Flashing

### Device Side

Build the project, flash it to the board, and run the monitor tool to check the serial output:

1. Run `. ./export.sh` to set up the IDF environment.
2. Install the Board Manager action helper once in the active ESP-IDF Python environment:

   ```bash
   pip install esp-bmgr-assist
   ```

3. From the repository root, enter this example directory and generate the product board configuration:

   ```bash
   cd examples/esp-idf/11_usb_extend_screen
   idf.py bmgr -b esp32_p4_wifi6_db_poe_eth
   ```

   `bmgr` selects the target automatically and generates `components/gen_bmgr_codes`. Do not run `idf.py set-target` separately.

4. Select a supported landscape display configuration with its Board Manager amendment:

   ```bash
   idf.py bmgr -b esp32_p4_wifi6_db_poe_eth -a ../esp32_p4_dsi_overlay/7_dsi_touch_c
   ```

   This applies the 7-DSI-TOUCH-C 1024×600 landscape display amendment. The 10.1B display is not supported.

5. Run `idf.py -p PORT flash monitor` to build, flash, and monitor the project.

(To exit the serial monitor, press `Ctrl-]`.)

Refer to the **Getting Started Guide** for full instructions on configuring and using ESP-IDF to build projects.

> **Note:** The first configuration/build downloads managed components. Ensure an internet connection is available.

### PC Side

For preparation, refer to [windows_driver](./windows_driver/README.md).

![Demo](https://dl.espressif.com/AE/esp-iot-solution/p4_usb_extern_screen.gif)

## Troubleshooting

### The touchscreen controls the wrong display

1. Open **Control Panel** and select **Tablet PC Settings**.
2. Under the **Display** section, select **Setup**.
3. Follow the on-screen instructions to choose the correct extended display.

### Adjusting JPEG Image Quality

* Modify `CONFIG_USB_EXTEND_SCREEN_JPEG_QUALITY`. A higher value increases image quality but also consumes more memory per frame.

### Changing the Secondary Screen Resolution

* The resolution and RGB565/RGB888 framebuffer format come from the selected board's `display_lcd` configuration; they are also used to build the USB vendor string, HID coordinate range, frame validation, and JPEG output buffers.
* To use another panel, select a supported landscape panel amendment and regenerate the Board Manager configuration. The 10.1B display is not supported.

**Note:** The driver currently does not support portrait-oriented screens. Please use a screen designed for landscape mode.

### ESP32-P4 Chip Revisions and JPEG Alignment

* ESP-IDF firmware built for ESP32-P4 revisions `<3.0` and `>=3.0` is mutually incompatible. Board Manager selects the board and target, but it does not select the silicon revision. For a `<3.0` chip, enable `CONFIG_ESP32P4_SELECTS_REV_LESS_V3` with `idf.py menuconfig` and perform a clean rebuild; do not put this setting in generic board defaults.
* Hardware JPEG output is rounded to the input JPEG's MCU size: 8×8 for YUV444, 16×8 for YUV422, and 16×16 for YUV420. The example parses each JPEG header, reserves worst-case 16×16-aligned storage, and removes row padding before drawing. A visible resolution such as 1024×600 therefore does not need to be changed to 1024×608.
* Revisions `<3.0` also restrict some YUV-to-YUV conversions. This example decodes directly to the display's RGB565 or RGB888 framebuffer format and does not use those restricted combinations.

### Adjusting Image Output Frame Rate

* Modify `CONFIG_USB_EXTEND_SCREEN_MAX_FPS`. Lowering this value effectively reduces USB bandwidth usage. If USB audio stuttering occurs, consider decreasing this value.

### Modifying the Maximum Frame Size

* Modify `CONFIG_USB_EXTEND_SCREEN_FRAME_LIMIT_B` to limit the maximum image size received from the PC driver.

### Disabling Touchscreen Functionality

* Set `CONFIG_HID_TOUCH_ENABLE` to `n`.
* This option is only available when the selected board provides `lcd_touch`.

### Disabling Audio Functionality

* Set `CONFIG_UAC_AUDIO_ENABLE` to `n`.
* This option is only available when the selected board provides Board Manager audio codec devices.

**Note:** If only the secondary screen function is enabled, change the PID to `0x2987`.
