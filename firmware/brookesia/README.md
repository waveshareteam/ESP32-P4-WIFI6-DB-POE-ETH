# ESP-Brookesia Factory Firmware

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5.4-E7352C?logo=espressif)](https://github.com/espressif/esp-idf/releases/tag/v5.5.4)
[![Target](https://img.shields.io/badge/target-ESP32--P4-1F6FEB)](https://www.espressif.com/en/products/socs/esp32-p4)
[![LVGL](https://img.shields.io/badge/LVGL-9.4.0-2AA9E0?logo=lvgl)](https://lvgl.io/)

English | [简体中文](README_CN.md)

This directory contains the source factory firmware for the
**Waveshare ESP32-P4-WIFI6-Touch-LCD-4B**. It turns the 720 x 720 touch display
into an ESP-Brookesia application shell and integrates the board display,
touch, audio, camera, SD card, Ethernet, relays, RS485, and ESP32-C6 hosted
Wi-Fi paths.

> [!IMPORTANT]
> This is board-specific firmware, not a generic ESP-Brookesia example. Use
> ESP-IDF v5.5.4 and the pinned component manifests in this project. The
> ESP32-P4 target, partition table, BSP revision, and C6 hosted firmware form a
> compatibility set and should be changed together only after hardware testing.

## Platform baseline

| Layer | Current project baseline |
| --- | --- |
| Host MCU | ESP32-P4, 32 MB flash, 32 MB PSRAM |
| Display / touch | 4-inch 720 x 720 MIPI DSI ST7703 / GT911 |
| Framework | ESP-IDF v5.5.4 (`idf_component.yml` requires `>=5.5`) |
| UI | LVGL 9.4.0 with local ESP-Brookesia core 0.6.0-beta2 |
| Board component | `waveshare/esp32_p4_wifi6_touch_lcd_4b` pinned to commit `e7731ec5d96a038bf2a1390c9f582dc62a3f36a2` |
| LVGL integration | Main UI uses `espressif/esp_lvgl_adapter` 0.6.4 with Board Manager display and touch handles |
| Wireless | ESP32-C6 coprocessor over SDIO using ESP-Hosted / `esp_wifi_remote` |
| Voice assistant | `espressif/esp_xiaozhi` 0.1.1, ESP-SR 2.4.6, Xiaozhi fonts 1.6.0 |

The pinned BSP comes from
[`waveshareteam/Waveshare-ESP32-components#178`](https://github.com/waveshareteam/Waveshare-ESP32-components/pull/178).
Keep the Git dependency until an equivalent released component is available.
The firmware obtains panel and touch handles through Board Manager, then
connects them to LVGL through `esp_lvgl_adapter`. VideoPlayer temporarily uses
the adapter's Dummy Draw path for full-screen video and returns to LVGL rendering
when playback closes.

Hardware pin details and the P4/C6 compatibility notes live in
[`docs/board.md`](../../docs/board.md) and
[`docs/p4-c6-hosted-wifi.md`](../../docs/p4-c6-hosted-wifi.md).

## Applications

The launcher installs the following applications. The PNGs below are the same
source icons used to generate the compiled LVGL assets.

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
    <td align="center"><img src="components/HardwareApps/assets/img_app_ethernet.png" width="64" alt="Ethernet icon"><br><b>Ethernet</b></td>
    <td align="center"><img src="components/HardwareApps/assets/img_app_relay_control.png" width="64" alt="Relay Control icon"><br><b>Relay Control</b></td>
    <td align="center"><img src="components/HardwareApps/assets/img_app_rs485.png" width="64" alt="RS485 icon"><br><b>RS485</b></td>
    <td align="center"><img src="components/brookesia_app_squareline_demo/assets/esp_brookesia_app_icon_launcher_squareline_112_112.png" width="64" alt="Squareline icon"><br><b>Squareline</b></td>
  </tr>
</table>

| Application | What it demonstrates | Runtime dependency |
| --- | --- | --- |
| AIChats | Network check, Xiaozhi activation, WakeNet wake word, VAD, Opus uplink/downlink, server transcript and TTS | Wi-Fi or Ethernet, `model` and `storage` images, ES7210/ES8311 |
| Calculator | Touch-friendly calculator built as a Brookesia phone app | None beyond display/touch |
| DrawPanel | Drawing in a fixed canvas region without dragging the entire app at the edge | Display/touch |
| SpecAnalyzer | Single-channel FFT spectrum from the FC microphone | Audio ADC channel 0 |
| MusicPlayer | Up to five local MP3 tracks with a spectrum visualizer | SPIFFS `storage`, ES8311 output |
| Settings | WLAN scan/connect, sound, display brightness, and device information pages | ESP32-C6 hosted Wi-Fi and board services |
| Squareline | Registry-installed Squareline Studio UI demo and application template | Display/touch |
| Camera | Centered 1:1 MIPI CSI preview with PPA | Supported OV5647 sensor mode and `esp_video` |
| VideoPlayer | Full-screen SD-card AVI/MJPEG playback with audio when available | SD card, JPEG/PPA, ES8311 output |
| Ethernet | IP101 link, IPv4, netmask, gateway, MAC, speed/duplex, and PHY status | On-board IP101 |
| Relay Control | Independent active-high control of GPIO32 and GPIO46 with NVS state restore | Relay outputs and NVS |
| RS485 | Periodic transmit and receive terminal with counters and a clear action | UART1 and board transceiver |

## Build

### Prerequisites

- ESP-IDF v5.5.4 with the `esp32p4` toolchain installed.
- Git and network access to the ESP Component Registry and the pinned Waveshare
  component repository on the first dependency resolution.
- An ESP32-P4-WIFI6-Touch-LCD-4B and a data-capable USB cable for flashing.
- Enough disk space for managed components and the generated SPIFFS/model
  images.

Activate the intended ESP-IDF environment first and verify it explicitly:

```sh
idf.py --version
idf.py -C firmware/brookesia set-target esp32p4
idf.py -C firmware/brookesia build
```

The checked-in `sdkconfig.defaults` selects the ESP32-P4 target, 32 MB flash,
hex PSRAM, the 720 x 720 panel, OV5647 camera mode, ESP32-C6 hosted transport,
and the "Ni Hao Xiao Zhi" WakeNet model. Do not edit files under
`managed_components/`; update a component manifest and let the IDF Component
Manager regenerate them.

With local components, the generated `dependencies.lock` contains absolute
component paths from the current build host. It is therefore ignored and is a
local build aid, not a publication artifact. Run `idf.py reconfigure` (or the
first `build`) to generate it in a new environment. Keep reproducible BSP
commits and component ranges in `idf_component.yml`, and do not publish a
host-specific lock file.

### First flash and partition changes

The first installation must write the bootloader, partition table, application,
ESP-SR model, and SPIFFS resources:

```sh
idf.py -C firmware/brookesia -p PORT flash monitor
```

Use a full erase before flashing when moving partition offsets/sizes, changing
from an older layout, or investigating a stale model/filesystem image:

```sh
idf.py -C firmware/brookesia -p PORT erase-flash flash monitor
```

After the partition layout, ESP-SR model, and `spiffs/` assets are already on
the device, application-only iterations are faster:

```sh
idf.py -C firmware/brookesia -p PORT app-flash monitor
```

`app-flash` does **not** update `srmodels.bin`, `storage.bin`, or the partition
table. Return to `flash` after changing `partitions.csv`, WakeNet/model Kconfig,
the Xiaozhi font component, bundled music, or prompt audio.

## Flash layout and resources

The current table intentionally ends at 16 MiB even though the board has 32 MB
flash. This keeps full-flash data smaller and keeps the memory-mapped ESP-SR
image in the first 16 MiB while Quad Flash 32-bit addressing is disabled.

| Partition | Offset | Size | Purpose |
| --- | ---: | ---: | --- |
| `nvsfactory` | `0x009000` | 200 KiB | Factory/configuration NVS |
| `nvs` | `0x03B000` | 840 KiB | Runtime NVS, including app state |
| `otadata` | `0x10D000` | 8 KiB | OTA selection metadata |
| `phy_init` | `0x10F000` | 4 KiB | PHY initialization data |
| `model` | `0x110000` | 960 KiB | Generated ESP-SR `srmodels.bin` |
| `factory` | `0x200000` | 12 MiB | Main firmware image |
| `storage` | `0xE00000` | 2 MiB | Generated SPIFFS resources |

Do not move `model` behind a large `factory` or `storage` partition without
also validating the platform's flash-MMAP and 32-bit-address configuration.
Simply detecting 32 MB flash does not make a partition above 16 MiB readable by
every memory-mapped consumer.

`storage.bin` is generated from `spiffs/` and also stages the 30 px common font
from `78/xiaozhi-fonts` as `/spiffs/xiaozhi/font.bin`. Runtime resources include:

- Up to five locally supplied MusicPlayer MP3 files under `/spiffs/music`; these remain ignored until redistributable assets and licenses are selected.
- Xiaozhi digit, activation, and success OGG prompts under `/spiffs/xiaozhi`.
- The Xiaozhi common Chinese/Latin font loaded into PSRAM at runtime.

## AIChats / Xiaozhi

AIChats is the launcher name of `components/XiaozhiApp`. It uses the managed
`esp_xiaozhi` protocol component while retaining board-specific UI, audio,
activation, and lifecycle integration.

### First-use flow

1. Open AIChats. Its UI is created immediately while readiness checks continue
   in the background.
2. The app accepts either a valid station IPv4 address or an Ethernet IPv4
   address. It does not run its own provisioning portal; configure WLAN in
   Settings or connect Ethernet.
3. If the device is not associated with a Xiaozhi account, the activation code
   is shown and announced. Add the device in the Xiaozhi control panel and
   enter that code.
4. After successful activation, the local success prompt is played and the
   transport connects.
5. When the UI reaches standby, say the configured wake phrase (the default
   model is "Ni Hao Xiao Zhi") and continue the conversation after the reply.

If the log reports `No speech model found` or `contains no compatible models`,
rebuild and perform a full flash so the partition table and
`srmodels/srmodels.bin` are both written. Application-only flashing cannot fix
this condition.

### Compile-time UI language

The local status text defaults to English. Change it with:

```text
idf.py menuconfig
  Xiaozhi Application
    Local UI language
      English
      Simplified Chinese
```

The corresponding options are
`CONFIG_XIAOZHI_APP_UI_LANGUAGE_ENGLISH` and
`CONFIG_XIAOZHI_APP_UI_LANGUAGE_SIMPLIFIED_CHINESE`. This setting affects only
firmware-owned status text and fallback activation instructions. Conversation
text, TTS, and messages returned by the server are displayed or played as
received; selecting the UI language does not translate the service response.

To change the maintained product default, replace the English symbol in
`sdkconfig.defaults` with the Simplified Chinese symbol and keep only one
choice set to `y`. Existing build directories retain their generated
`sdkconfig`; use `menuconfig` or regenerate `sdkconfig` after changing the
defaults file.

Add or revise local strings in `components/XiaozhiApp/XiaozhiLocalization.*`.
Keep server-provided transcript and activation text out of that table.

### Product tool extension hook

`registerXiaozhiDeveloperTools()` in
`components/XiaozhiApp/XiaozhiDeveloperTools.cpp` is the intended MCP extension
point. It runs for each new MCP engine before transport startup and currently
registers no tools. The TODO documents candidate product tools for:

- Setting display brightness.
- Setting output volume.
- Capturing a camera frame for a vision request.

Implement only callable tools: create the MCP schema and callback, validate
arguments, add the tool to the engine, and report the real operation result.
Do not advertise placeholder tools through `tools/list`. Dispatch blocking
hardware work to an application task rather than blocking the protocol
callback, and retain/copy camera buffers until the result builder has consumed
them.

### Voice audio contract

AIChats configures the board audio bus at 24 kHz, extracts two microphone
channels and one echo-reference channel, resamples them to 16 kHz, and feeds
ESP-SR in `MMR` order. The Opus uplink is 16 kHz; server playback is resampled
to the 24 kHz device rate when required.

| ES7210 serialized TDM slot | Board-observed source | Use |
| ---: | --- | --- |
| 0 | Physical MIC1 | SpecAnalyzer FC spectrum; AIChats microphone 1 |
| 1 | Physical MIC3 | AIChats echo reference |
| 2 | Physical MIC2 | AIChats microphone 2 |
| 3 | Physical MIC4 | Captured in the full frame but not used by these apps |

TDM slot masks and physical ES7210 microphone gain masks are different
namespaces. Never pass a serialized slot mask as a physical microphone mask.
SpecAnalyzer analyzes the FC input on channel 0 only. AIChats uses the full
four-slot ES7210 TDM frame for microphone and echo-reference processing.

## Hardware application notes

### Relay persistence

Relay Control stores a two-bit state in NVS namespace `relay_control`, key
`output_state`. App initialization restores GPIO32/GPIO46 after a reboot, and
opening the UI reads the current GPIO state so killing and reopening the app
does not reset the switches. Erasing NVS intentionally returns both outputs to
the default off state.

### Ethernet

The Ethernet app starts the on-board IP101 (PHY address 1) with DHCP and shows
link state, IPv4 address, netmask, gateway, MAC address, speed/duplex, and PHY
identity. The pin assignment is project-specific glue in `components/bsp_extra`
and is documented in [`docs/board.md`](../../docs/board.md).

### RS485

The terminal uses UART1 at 115200 8N1 on TX GPIO47 / RX GPIO48. Auto-send is
enabled by default and transmits once per second:

```text
Hi,this is Waveshare ESP32-P4-WIFI6-Touch-LCD-4B!
```

The receive window escapes non-printable bytes and keeps byte/frame counters.
On the reviewed baseboard V1.3 schematic, `/RE` and `DE` are tied together and
pulled high, so the transceiver can remain in transmit mode and the receive
path may be unavailable. Do not short A and B together for loopback testing;
that collapses the differential bus. Correct or isolate direction control
before connecting another actively driven RS485 adapter.

## Camera and media requirements

### Camera

The board configuration selects OV5647 MIPI RAW8 800 x 800 at 50 FPS and an
800 x 1280 RGB565 display. Camera initializes the device through board manager
on first use and retains that reference until reboot. Three V4L2 MMAP buffers
feed PPA for a 1:1 copy into two PSRAM image buffers. An LVGL image object
centers the preview, with 240-pixel black bands above and below. No scaling or
cropping is performed; sensor modes larger than the display are rejected.
Capture buffers are returned after the copy, and the GUI timer displays the
latest completed frame.

LVGL and existing touch input remain active. Swipe up from the bottom edge to
return home; resuming restarts capture. Errors appear on the app screen. DEBUG
logs report preview FPS; 50 FPS is the sensor mode rate, not a measured display
rate. Build and hardware validation are still required.

### VideoPlayer

VideoPlayer mounts the SD card and scans exactly `/sdcard/avi`. Filenames are
otherwise unrestricted and the `.avi` extension is matched case-insensitively.
MP4 files and AVI files in the SD root are not enumerated. The player decodes
AVI/MJPEG video to the full display, loops through discovered files, and sends
AVI audio to the board codec when audio initialization succeeds; otherwise it
continues video-only.

Video playback uses `esp_video_player_service` and `esp_playlist` to manage AVI
playback. A project-local `esp_video_render` backend writes complete frames into
the adapter's triple-buffered Dummy Draw buffers; LVGL screen drawing is bypassed
while video is active, while adapter input processing remains available for the
swipe-to-exit gesture. Closing playback disables Dummy Draw and redraws the LVGL
screen. The board display uses RGB565; decoding details, aspect handling, and
throughput still need board validation.

Audio output is managed by `esp_video_player_service` and initialized through
the Board Manager audio DAC. SD card, DAC, or service initialization failures
appear on the app screen. Supported formats and A/V synchronization still need
board validation. Scanned files are imported into `esp_playlist`, which manages
the current item and list repeat. Full paths must be shorter than `CONFIG_ESP_PLAYLIST_URL_MAX`;
overlong paths return an error rather than opening a truncated path. Swipe up to
exit. The player service handles pause and close before render resources are
released.

### MusicPlayer

MusicPlayer enumerates up to five locally supplied MP3 files under
`/spiffs/music`. Album covers are omitted; the UI reuses prerecorded spectrum sequences.
The demo MP3 files are ignored until redistributable assets and licenses are
selected, so a clean public checkout may have an empty playlist. Before
release, add approved audio or document how integrators provide it. Replacing
or adding tracks requires rebuilding and flashing `storage.bin`; `app-flash`
alone is insufficient.

## Project structure

| Path | Ownership |
| --- | --- |
| `main/main.cpp` | Board startup, loading UI, phone shell, app installation, status events |
| `components/brookesia_core/` | Local Brookesia application framework and phone UI |
| `components/bsp_extra/` | Product-specific audio, backlight, relay, IP101, and RS485 glue |
| `components/XiaozhiApp/` | AIChats UI, activation, audio pipeline, protocol adapter, MCP hook |
| `components/HardwareApps/` | Relay, Ethernet, and RS485 applications |
| `components/<App>/` | Individual launcher applications and compiled assets |
| `spiffs/` | Source files staged into the `storage` partition image |
| `archive/` | Inactive historical implementation retained for reference; not installed |
| `partitions.csv` | Factory firmware flash layout |
| `sdkconfig.defaults` | Maintained project defaults |
| `managed_components/` | Generated Component Manager checkout; do not edit directly |
| `build/` | Generated output; never treat as source |

## Adding an application

Use a small existing component such as `brookesia_app_calculator` as the basic
lifecycle example, and `HardwareApps` or `XiaozhiApp` when background tasks and
shared hardware are involved.

1. Create `components/<AppName>/` with `CMakeLists.txt`, `idf_component.yml`,
   headers, implementation, and an `assets/` directory.
2. Derive from `esp_brookesia::systems::phone::App` and implement the required
   `run()` and `back()` methods. Override `init()`, `deinit()`, `close()`,
   `pause()`, `resume()`, and `cleanResource()` only as needed for resources
   owned by the application.
3. Keep LVGL object creation/mutation on the LVGL thread or within a short
   display lock. Publish data from worker tasks and let a timer/async callback
   update the UI.
4. Provide the compiled LVGL icon and keep its source PNG beside it so launcher
   artwork and documentation remain reviewable.
5. Add required managed dependencies to the component manifest instead of
   copying third-party components.
6. Request the singleton and install it with `phone->installApp()` in
   `main/main.cpp`, or use the existing registry mechanism for a registry-owned
   app.
7. Test repeated open, background, resume, close, and reboot cycles. Release
   every timer, task, queue, codec session, SD handle, and LVGL object owned by
   the app.

## Common pitfalls

- **Long LVGL lock scopes freeze animation.** Hold the display/LVGL lock only
  around UI operations. Do networking, SD scans, codec startup, NVS access,
  model loading, and app construction outside it.
- **A successful app build does not update data images.** Model, font, prompt,
  music, and partition changes require a full `flash`.
- **The model partition is placement-sensitive.** Keep the ESP-SR image below
  the current 16 MiB addressing boundary unless flash-MMAP/32-bit addressing is
  enabled and verified on this board.
- **ES7210 labels are not DMA slot numbers.** Use the observed
  `[MIC1, MIC3(echo), MIC2, MIC4]` serialized order and separate physical gain
  masks from TDM slot masks.
- **Audio apps share one physical bus.** Stop or hand off codec sessions when
  switching between voice capture, music, and video; do not independently
  reconfigure paired I2S channels.
- **Camera dimensions come from the sensor mode.** Preview uses a centered 1:1 copy;
  do not assume the CSI node accepts the LCD dimensions.
- **VideoPlayer is AVI-specific.** Use `/sdcard/avi/*.avi`; MP4 is not supported
  by the current enumerator/player.
- **P4 has no native Wi-Fi radio.** Host components must remain compatible with
  the firmware running on the ESP32-C6 coprocessor. A host-only dependency
  upgrade can break wireless operation even when it compiles.
- **The BSP is pinned to an unreleased commit.** Review upstream PR/release
  changes before updating it, especially Board Manager device handles and
  configuration structures.
- **RS485 receive is hardware-revision dependent.** Check transceiver direction
  wiring before diagnosing the terminal software.

## Diagnostics

The periodic SRAM/PSRAM report is compiled out by default through
`EXAMPLE_SHOW_MEM_INFO = false` in `main/main.cpp`. Temporarily enable it only
when investigating memory pressure; the surrounding diagnostic logic remains
available.

Useful runtime evidence includes the first real error, the partition/model
load lines, camera V4L2 format, Ethernet link events, and the four ES7210 TDM
peaks. Include those lines, the board revision, ESP-IDF version, and the exact
reproduction sequence in bug reports.

## Contributing, support, and release checklist

Read the repository-level [contribution guide](../../CONTRIBUTING.md),
[support policy](../../SUPPORT.md), and
[security policy](../../SECURITY.md) before opening a change. Keep reusable BSP
or driver fixes in the appropriate upstream component where possible; keep
only product composition and hardware-specific glue in this firmware.

The local Brookesia core retains its
[Apache-2.0 license](components/brookesia_core/license.txt). That license applies
to that component; it does not select a license for the repository as a whole.

Before publishing a release:

- Build from a clean checkout with the intended ESP-IDF version and regenerate
  dependencies from the committed component manifests.
- Full-flash and verify display/touch, both microphones, speaker, Wi-Fi,
  Ethernet, camera, SD/video, relays, RS485 constraints, and Xiaozhi activation
  plus a complete wake/conversation/stop cycle.
- Record the compatible ESP32-C6 hosted firmware revision.
- Package the complete flash set, not only the application binary.
- Remove generated build output and inspect public text for machine-specific
  paths, usernames, credentials, activation identifiers, and Wi-Fi secrets.
- Select and add a repository-level `LICENSE.txt` before public distribution,
  then retain all component license files, SPDX headers, and required
  third-party notices. Until that license is added, do not assume the entire
  repository is covered by any one component's license.
