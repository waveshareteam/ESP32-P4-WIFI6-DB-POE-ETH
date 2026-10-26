<h1 align="center">ESP-Brookesia Factory Firmware</h1>

<p align="center">
  <a href="https://github.com/espressif/esp-idf/releases"><img src="https://img.shields.io/badge/ESP--IDF-v5.5.6%20%7C%20v6.1-E7352C?logo=espressif" alt="ESP-IDF"></a>
  <a href="https://www.espressif.com/en/products/socs/esp32-p4"><img src="https://img.shields.io/badge/target-ESP32--P4-1F6FEB" alt="Target"></a>
  <a href="https://lvgl.io/"><img src="https://img.shields.io/badge/LVGL-9.4.0-2AA9E0?logo=lvgl" alt="LVGL"></a>
</p>

<p align="center">English | <a href="README_CN.md">简体中文</a></p>

<p align="center">
  Factory firmware for the Waveshare ESP32-P4-WIFI6-DB-Touch-LCD-7C, built on the ESP-Brookesia phone-style launcher, with multimedia, camera, RS485/RS232, CAN and Xiaozhi AI voice assistant apps.
</p>

---

## 🧩 Hardware platform

<table>
  <tr><th>Module</th><th>Chip</th><th>Description</th></tr>
  <tr><td>SoC</td><td>ESP32-P4NRW32X</td><td>Dual-core RISC-V at 400 MHz, 32 MiB in-package PSRAM, external 32 MiB flash</td></tr>
  <tr><td>Display</td><td>EK79007</td><td>7-inch 1024 x 600 MIPI-DSI panel</td></tr>
  <tr><td>Touch</td><td>GT911</td><td>Capacitive touch</td></tr>
  <tr><td>Camera</td><td>OV5647</td><td>5 MP MIPI-CSI sensor</td></tr>
  <tr><td>Audio</td><td>ES8389</td><td>Low-power audio codec, two differential microphones</td></tr>
  <tr><td>Wireless</td><td>ESP32-C5-MINI-1</td><td>Dual-band Wi-Fi 6 and Bluetooth LE co-processor over SDIO</td></tr>
  <tr><td>Storage</td><td>Micro SD card slot</td><td>4-bit SDMMC, mounted at <code>/sdcard</code></td></tr>
  <tr><td>RTC</td><td>PCF85063ATL</td><td>Low-power real-time clock</td></tr>
  <tr><td>CAN</td><td>TJA1051T/3/1J</td><td>High-speed CAN transceiver</td></tr>
  <tr><td>RS232</td><td>SP3232EEN</td><td>3.3 V RS-232 transceiver</td></tr>
  <tr><td>RS485</td><td>SP3485EN</td><td>3.3 V half-duplex RS-485 transceiver</td></tr>
</table>

> See the [board user guide](../../docs/en/hw-reference/user-guide-esp32-p4-wifi6-db-touch-lcd-7c.md) for more hardware details.
>
> Board support package (BSP): [BSP overview](../../components/bsp/esp32_p4_wifi6_db_touch_lcd_7c/README.md) | [API reference](../../components/bsp/esp32_p4_wifi6_db_touch_lcd_7c/API.md)

## 🛠️ Build and flash

You need ESP-IDF v5.5.x or v6.1 and network access to the ESP Component Registry. Run from this directory:

```sh
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

> [!NOTE]
> The first installation must use a full `flash` so the partition table, ESP-SR models (`srmodels.bin`) and SPIFFS resources (`storage.bin`) are written. If you later change only application code, `idf.py -p PORT app-flash monitor` is faster.

### Flash partitions

The 32 MiB flash layout is defined in `partitions.csv`; the partition table is at `0x8000`:

<table>
  <tr><th>Name</th><th>Type</th><th>Offset</th><th>Size</th><th>Purpose</th></tr>
  <tr><td><code>nvsfactory</code></td><td>data / nvs</td><td><code>0x9000</code></td><td>200 KiB</td><td>Factory NVS data</td></tr>
  <tr><td><code>nvs</code></td><td>data / nvs</td><td><code>0x3B000</code></td><td>840 KiB</td><td>Wi-Fi, app settings and Xiaozhi state</td></tr>
  <tr><td><code>otadata</code></td><td>data / ota</td><td><code>0x10D000</code></td><td>8 KiB</td><td>OTA data (reserved)</td></tr>
  <tr><td><code>phy_init</code></td><td>data / phy</td><td><code>0x10F000</code></td><td>4 KiB</td><td>PHY calibration data</td></tr>
  <tr><td><code>model</code></td><td>data / spiffs</td><td><code>0x110000</code></td><td>960 KiB</td><td>ESP-SR speech models (<code>srmodels.bin</code>)</td></tr>
  <tr><td><code>factory</code></td><td>app / factory</td><td><code>0x200000</code></td><td>8 MiB</td><td>Application</td></tr>
  <tr><td><code>storage</code></td><td>data / spiffs</td><td><code>0xA00000</code></td><td>3 MiB</td><td>Music, Xiaozhi prompts and font (<code>storage.bin</code>)</td></tr>
</table>

## 📱 Applications

<table>
  <tr>
    <td align="center"><img src="components/DrawPanel/assets/img_app_drawpanel.png" width="64" alt="DrawPanel icon"><br><b>DrawPanel</b></td>
    <td align="center"><img src="components/brookesia_app_squareline_demo/assets/esp_brookesia_app_icon_launcher_squareline_112_112.png" width="64" alt="Squareline icon"><br><b>Squareline</b></td>
    <td align="center"><img src="components/brookesia_app_calculator/assets/img_app_calculator.png" width="64" alt="Calculator icon"><br><b>Calculator</b></td>
    <td align="center"><img src="components/SpecAnalyzer/assets/img_app_specanalyzer.png" width="64" alt="SpecAnalyzer icon"><br><b>SpecAnalyzer</b></td>
    <td align="center"><img src="components/MusicPlayer/assets/img_app_musicplayer.png" width="64" alt="MusicPlayer icon"><br><b>MusicPlayer</b></td>
    <td align="center"><img src="components/Camera/assets/img_app_camera.png" width="64" alt="Camera icon"><br><b>Camera</b></td>
  </tr>
  <tr>
    <td align="center"><img src="components/VideoPlayer/assets/img_app_vedioplayer.png" width="64" alt="VideoPlayer icon"><br><b>VideoPlayer</b></td>
    <td align="center"><img src="components/UartViewer/assets/img_app_uart_tool.png" width="64" alt="UART icon"><br><b>UART</b></td>
    <td align="center"><img src="components/CANViewer/assets/img_app_can_tool.png" width="64" alt="CAN icon"><br><b>CAN</b></td>
    <td align="center"><img src="components/Settings/assets/img_app_settings.png" width="64" alt="Settings icon"><br><b>Settings</b></td>
    <td align="center"><img src="components/XiaozhiApp/assets/img_app_xiaozhi.png" width="64" alt="AIChats icon"><br><b>AIChats</b></td>
  </tr>
</table>

<table>
  <tr><th>Application</th><th>Function</th><th>Component directory</th></tr>
  <tr><td>DrawPanel</td><td>Fixed canvas; drawing near the edges does not drag the app</td><td><code>components/DrawPanel/</code></td></tr>
  <tr><td>Squareline</td><td>Registry-installed Squareline Studio UI demo and app template</td><td><code>components/brookesia_app_squareline_demo/</code></td></tr>
  <tr><td>Calculator</td><td>Touch-friendly Brookesia calculator</td><td><code>components/brookesia_app_calculator/</code></td></tr>
  <tr><td>SpecAnalyzer</td><td>FFT spectrum of the on-board microphones</td><td><code>components/SpecAnalyzer/</code></td></tr>
  <tr><td>MusicPlayer</td><td>Plays local MP3 files from <code>/spiffs/music</code> with a spectrum view</td><td><code>components/MusicPlayer/</code></td></tr>
  <tr><td>Camera</td><td>OV5647 MIPI CSI preview, scaled by the PPA to fill the landscape panel</td><td><code>components/Camera/</code></td></tr>
  <tr><td>VideoPlayer</td><td>Full-screen AVI/MJPEG playback from the SD card, with audio when available</td><td><code>components/VideoPlayer/</code></td></tr>
  <tr><td>UART</td><td>RS485/RS232 serial terminal with selectable baud rate, frame format and HEX send/receive</td><td><code>components/UartViewer/</code></td></tr>
  <tr><td>CAN</td><td>TWAI terminal, 125 kbit/s to 1 Mbit/s; reports bus state</td><td><code>components/CANViewer/</code></td></tr>
  <tr><td>Settings</td><td>WLAN scan/connect, sound, screen brightness, SNTP time and device information</td><td><code>components/Settings/</code></td></tr>
  <tr><td>AIChats</td><td>Network check, Xiaozhi activation, WakeNet wake word, VAD, Opus uplink/downlink, server text, emotions and TTS</td><td><code>components/XiaozhiApp/</code></td></tr>
</table>

In addition, `components/brookesia_core/` is the local ESP-Brookesia core, and `components/bsp_extra/` provides audio, Wi-Fi service and board peripheral extensions on top of the BSP.

## 🚀 Quick start

1. Power on the board and wait for the boot screen to finish loading the home screen.
2. Open **Settings → WLAN** and connect to Wi-Fi; the clock is synchronized automatically once connected.
3. Open **AIChats**. On first use it shows and reads out an activation code; add the device in the [Xiaozhi console](https://xiaozhi.me) and enter the code.
4. Copy MJPEG-encoded `.avi` videos to the `avi/` folder on the SD card, insert the card and play them in **VideoPlayer**.
5. Use the **UART** and **CAN** apps with external RS485/RS232 or CAN devices to send and receive data; parameters are set inside each app.

## 🎙️ Voice interaction

Once AIChats is activated and in standby, say the wake word **"Ni Hao Xiao Zhi" (你好小智)** or tap the emoji on screen to start a conversation.

<table>
  <tr><th>Situation</th><th>Behavior</th></tr>
  <tr><td>While Xiaozhi speaks</td><td>Half-duplex, it does not listen; the wake word interrupts it and starts listening again, tapping the screen does not</td></tr>
  <tr><td>While Xiaozhi answers</td><td>The screen shows the text and emotion returned by the server</td></tr>
  <tr><td>After each answer</td><td>Listening resumes automatically for a continuous conversation</td></tr>
  <tr><td>Server ends the session</td><td>Returns to standby; say the wake word again to start a new conversation</td></tr>
  <tr><td>Close / switch apps</td><td>Closing AIChats disconnects and reopening reconnects automatically; switching to another app keeps the connection</td></tr>
</table>

> [!NOTE]
> The firmware does not register any MCP tools yet, so Xiaozhi cannot control device functions such as volume, brightness or the camera by voice.

### Change the wake word

The default wake word is "Ni Hao Xiao Zhi" (`wn9l_nihaoxiaozhi_tts3`). Change it in `menuconfig`:

```text
idf.py menuconfig
  ESP Speech Recognition
    Load Multiple Wake Words (WakeNet9)
      [ ] 你好小智 (wn9l_nihaoxiaozhi_tts3)   <- deselect
      [*] Hi,ESP (wn9_hiesp)                  <- select the new wake word
```

- Select only one wake word: the firmware loads only the first WakeNet model it finds.
- ESP32-P4 supports WakeNet9 / WakeNet9l; do not select models from the WakeNet9s menu.
- Wake-word models are packed into `srmodels.bin`, so rebuild and run a full `flash` after the change; `app-flash` does not apply it.
- The model must fit in the 960 KiB `model` partition.

### Change the Xiaozhi UI language

The local AIChats UI text defaults to English and can be switched to Simplified Chinese:

```text
idf.py menuconfig
  Xiaozhi Application
    Local UI language  (English / Simplified Chinese)
```

Rebuild and flash after the change; `app-flash` is enough when only the application changed. This option only affects the firmware's built-in status text and activation hints; conversations, TTS and activation messages from the server are unchanged. Local strings are in `components/XiaozhiApp/XiaozhiLocalization.cpp`.
