# ESP-Brookesia Factory Firmware

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5.4-E7352C?logo=espressif)](https://github.com/espressif/esp-idf/releases/tag/v5.5.4)
[![Target](https://img.shields.io/badge/target-ESP32--P4-1F6FEB)](https://www.espressif.com/en/products/socs/esp32-p4)
[![LVGL](https://img.shields.io/badge/LVGL-9.4.0-2AA9E0?logo=lvgl)](https://lvgl.io/)

English | [简体中文](README_CN.md)

This directory contains the ESP-Brookesia firmware source for the
**Waveshare ESP32-P4-WIFI6-DB-POE-ETH**. It supports the display, touch, audio,
camera, SD card, and ESP32-C5 hosted Wi-Fi. The board configuration also
includes an IP101 RMII Ethernet device.

## Platform configuration

| Item | Configuration |
| --- | --- |
| Host MCU | ESP32-P4NRW32X, 32 MB flash, 32 MB PSRAM |
| Display / touch | Supports Waveshare [5](https://www.waveshare.net/shop/5-DSI-TOUCH-A.htm), [7](https://www.waveshare.net/shop/7-DSI-TOUCH-A.htm), [8](https://www.waveshare.net/shop/8-DSI-TOUCH-A.htm), and [10.1](https://www.waveshare.net/shop/10.1-DSI-TOUCH-A.htm)-inch panels |
| Framework | ESP-IDF v5.5.4; `idf_component.yml` requires `>=5.5` |
| UI | LVGL 9.4.0 with the local ESP-Brookesia core 0.6.0-beta2 |
| Board Manager | `esp32_p4_wifi6_db_poe_eth`, `espressif/esp_board_manager` 0.7.4 |
| LVGL adapter | `espressif/esp_lvgl_adapter` 0.6.4 |
| Wireless | ESP32-C5 over SDIO slot 1 using ESP-Hosted / `esp_wifi_remote` |
| Voice assistant | `espressif/esp_xiaozhi` 0.1.1, ESP-SR 2.4.6, and xiaozhi-fonts 1.6.0 |

The Board Manager board definition provides the display, touch, ES8311, SD card,
camera, backlight, and IP101 devices. See the
[hardware reference guide](../../docs/zh_CN/hw-reference/user-guide-esp32-p4-wifi6-db-poe-eth.md)
for interfaces and pin assignments. The ESP32-P4/ESP32-C5 companion project is
[`15_wifi_iperf`](../../examples/esp-idf/15_wifi_iperf/).

## Applications

<table>
  <tr>
    <td align="center"><img src="components/XiaozhiApp/assets/img_app_xiaozhi.png" width="64" alt="AIChats icon"><br><b>AIChats</b></td>
    <td align="center"><img src="components/brookesia_app_calculator/assets/img_app_calculator.png" width="64" alt="Calculator icon"><br><b>Calculator</b></td>
    <td align="center"><img src="components/draw/assets/img_app_drawpanel.png" width="64" alt="DrawPanel icon"><br><b>DrawPanel</b></td>
    <td align="center"><img src="components/SpecAnalyzer/assets/img_app_specanalyzer.png" width="64" alt="SpecAnalyzer icon"><br><b>SpecAnalyzer</b></td>
    <td align="center"><img src="components/MusicPlayer/assets/img_app_musicplayer.png" width="64" alt="MusicPlayer icon"><br><b>MusicPlayer</b></td>
    <td align="center"><img src="components/Settings/assets/img_app_settings.png" width="64" alt="Settings icon"><br><b>Settings</b></td>
  </tr>
  <tr>
    <td align="center"><img src="components/Camera/assets/img_app_camera.png" width="64" alt="Camera icon"><br><b>Camera</b></td>
    <td align="center"><img src="components/VideoPlayer/assets/img_app_vedioplayer.png" width="64" alt="VideoPlayer icon"><br><b>VideoPlayer</b></td>
    <td align="center"><img src="components/brookesia_app_squareline_demo/assets/esp_brookesia_app_icon_launcher_squareline_112_112.png" width="64" alt="Squareline icon"><br><b>Squareline</b></td>
  </tr>
</table>

| Application | Function | Runtime dependency |
| --- | --- | --- |
| AIChats | Network check, Xiaozhi activation, WakeNet wake word, VAD, Opus uplink/downlink, server transcript and TTS | Wi-Fi, `model`/`storage` images, ES8311 |
| Calculator | Touch-friendly Brookesia calculator | Display and touch |
| DrawPanel | Drawing in a fixed canvas area without dragging the entire app at the edge | Display and touch |
| SpecAnalyzer | Single-channel FFT spectrum from the FC microphone | Audio ADC channel 0 |
| MusicPlayer | Up to five local MP3 tracks with a spectrum visualizer | SPIFFS `storage`, ES8311 output |
| Settings | WLAN scan/connect, sound, display brightness, and device information | ESP32-C5 hosted Wi-Fi and board services |
| Squareline | Registry-installed Squareline Studio UI demo and application template | Display and touch |
| Camera | Centered, aspect-preserving MIPI CSI preview using PPA | OV5647, `esp_video` |
| VideoPlayer | Full-screen SD-card AVI/MJPEG playback, with audio when available | SD card, JPEG/PPA, ES8311 output |

## Build

### Requirements

Use ESP-IDF v5.5.4 with the `esp32p4` toolchain. Network access to the ESP
Component Registry and Waveshare component repository is required. Use a
data-capable USB cable for flashing.

```sh
idf.py --version
idf.py -C firmware/brookesia set-target esp32p4
idf.py -C firmware/brookesia build
```

### First full flash and partition changes

The first installation must write the bootloader, partition table, application,
ESP-SR model, and SPIFFS resources:

```sh
idf.py -C firmware/brookesia -p PORT flash monitor
```

Erase the flash before a full flash when changing partition offsets or sizes,
upgrading from an older partition layout, or investigating stale model or
filesystem images:

```sh
idf.py -C firmware/brookesia -p PORT erase-flash flash monitor
```

When the partition layout, voice model, and `spiffs/` resources are unchanged,
you can flash only the application:

```sh
idf.py -C firmware/brookesia -p PORT app-flash monitor
```

`app-flash` does not update `srmodels.bin`, `storage.bin`, or the partition
table. Run a full `flash` after changing `partitions.csv`, WakeNet/model Kconfig,
xiaozhi-fonts, bundled music, or prompt audio.

## AIChats / Xiaozhi

AIChats uses `esp_xiaozhi` for the protocol. This project provides the UI,
activation, audio, and lifecycle integration.

First use:

1. Configure WLAN in Settings or connect Ethernet.
2. Open AIChats. If the device is not associated with an account, it displays
   and announces an activation code.
3. Add the device in the Xiaozhi control panel and enter the activation code.
4. When the page reaches standby, say the wake phrase. The default is
   “Ni Hao Xiao Zhi.”

If the log reports `No speech model found` or `contains no compatible models`,
rebuild and perform a full flash to write both the partition table and
`srmodels/srmodels.bin`.

The local UI language can be selected in this menu:

```text
idf.py menuconfig
  Xiaozhi Application
    Local UI language
      English
      Simplified Chinese
```

This option controls firmware-owned text only. It does not translate
conversation text, TTS, or activation messages returned by the server. Local
strings are in `components/XiaozhiApp/XiaozhiLocalization.*`.

The board audio bus uses ES8311 at 48 kHz, 16-bit, two-channel `FC,RE`. The
hardware has one working microphone. The AFE uses the `MN` layout: `FC` is the
microphone input, and the second I2S slot is ignored and is not an AEC reference.
AIChats converts captured audio to 16 kHz mono Opus for uplink; the playback
chain converts server audio as needed for 48 kHz output. SpecAnalyzer also uses
only the `FC` channel.

## Board hardware

### Ethernet

The Board Manager configuration includes the board's IP101 (PHY address 1)
RMII device. The launcher does not install separate Ethernet, Relay Control, or
RS485 applications.

## Camera and media

Camera uses the OV5647 MIPI RAW8 800 x 800, 50 FPS mode. Three V4L2 MMAP buffers
are scaled by PPA to fit the display area and copied into two PSRAM image
buffers. LVGL centers the preview without cropping. The displayed frame rate in
DEBUG logs must be confirmed on hardware.

VideoPlayer scans only `/sdcard/avi`; the `.avi` extension is case-insensitive.
AVI files in the SD card root and MP4 files are not enumerated. The player reads
AVI through `avi_player`, decodes MJPEG with `esp_new_jpeg`, then scales frames
with PPA and writes them directly to the LCD frame buffer. If audio
initialization fails, video playback continues silently.

MusicPlayer reads up to five MP3 files from `/spiffs/music`. After changing
tracks, regenerate and flash `storage.bin`; `app-flash` alone does not update it.

## Development notes

- The MCP tool entry point is `registerXiaozhiDeveloperTools()` in
  `components/XiaozhiApp/XiaozhiDeveloperTools.cpp`. Register only implemented
  tools that return real results; run long operations in an application task.
- New apps derive from `esp_brookesia::systems::phone::App` and implement
  `run()` and `back()`. Override other lifecycle functions according to
  resource ownership.
- Access LVGL objects on the LVGL thread or while holding the display lock
  briefly. On app shutdown, release its timers, tasks, queues, codec sessions,
  SD handles, and LVGL objects.
- During playback, VideoPlayer enables the adapter's Dummy Draw and pauses LVGL.
  On exit, it stops `avi_player` and resumes LVGL rendering.

## Project structure

| Path | Purpose |
| --- | --- |
| `CMakeLists.txt` | ESP-IDF project entry point |
| `main/main.cpp` | Initializes the Phone, installs apps, and handles Wi-Fi status events |
| `components/brookesia_core/` | Local Brookesia core implementation |
| `components/esp_hal_boards/` | Brookesia audio and display HAL adapters |
| `components/gen_bmgr_codes/` | Selects and generates Board Manager board configuration code |
| `components/{brookesia_app_calculator, brookesia_app_squareline_demo, draw, Camera, XiaozhiApp, VideoPlayer, MusicPlayer, SpecAnalyzer, Settings}/` | Launcher apps: calculator, Squareline demo, drawing, camera, Xiaozhi, video, music, spectrum analyzer, and settings |
| `spiffs/` | Music and Xiaozhi resources used to generate the `storage` partition image |
| `partitions.csv` | Flash partition layout |
| `sdkconfig.defaults` | Project default configuration |

## Notes

- Use the LVGL lock only for UI operations. Run networking, SD scans, audio
  initialization, NVS access, and model loading outside the lock.
- Model, font, prompt, music, and partition changes require a full `flash`.
- Audio apps share one physical bus. Stop or hand off the codec session when
  switching between apps.
- The host component must remain compatible with the coprocessor firmware on
  the ESP32-C5.
- Review Board Manager interface changes in upstream PRs/releases before
  updating the BSP.

`EXAMPLE_SHOW_MEM_INFO` in `main/main.cpp` is currently `true`, so periodic
SRAM/PSRAM statistics are enabled. The detailed log output is controlled
separately by `EXAMPLE_SHOW_MEM_INFO_LOG`.

When reporting an issue, include the board revision, ESP-IDF version, complete
reproduction steps, and the first actual error in the logs.

The local Brookesia core uses the
[Apache-2.0 license](components/brookesia_core/license.txt), which applies only
to that component. A repository-level license still needs to be selected before
public distribution; retain component licenses, SPDX headers, and third-party
notices.
