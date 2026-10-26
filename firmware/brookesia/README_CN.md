<h1 align="center">ESP-Brookesia 出厂固件</h1>

<p align="center">
  <a href="https://github.com/espressif/esp-idf/releases"><img src="https://img.shields.io/badge/ESP--IDF-v5.5.6%20%7C%20v6.1-E7352C?logo=espressif" alt="ESP-IDF"></a>
  <a href="https://www.espressif.com/en/products/socs/esp32-p4"><img src="https://img.shields.io/badge/target-ESP32--P4-1F6FEB" alt="Target"></a>
  <a href="https://lvgl.io/"><img src="https://img.shields.io/badge/LVGL-9.4.0-2AA9E0?logo=lvgl" alt="LVGL"></a>
</p>

<p align="center"><a href="README.md">English</a> | 简体中文</p>

<p align="center">
  Waveshare ESP32-P4-WIFI6-DB-Touch-LCD-7C</b> 的出厂固件，基于 ESP-Brookesia 手机风格桌面，集成多媒体、摄像头、RS485/RS232、CAN 和小智 AI 语音助手等应用。
</p>

---

## 🧩 硬件平台

<table>
  <tr><th>模块</th><th>芯片</th><th>说明</th></tr>
  <tr><td>主控</td><td>ESP32-P4NRW32X</td><td>双核 RISC-V 400 MHz，内置 32 MiB PSRAM，外置 32 MiB Flash</td></tr>
  <tr><td>显示</td><td>EK79007</td><td>7 英寸 1024 x 600 MIPI-DSI 屏</td></tr>
  <tr><td>触摸</td><td>GT911</td><td>电容触摸</td></tr>
  <tr><td>摄像头</td><td>OV5647</td><td>500 万像素 MIPI-CSI 传感器</td></tr>
  <tr><td>音频</td><td>ES8389</td><td>低功耗音频编解码器，两路差分麦克风</td></tr>
  <tr><td>无线</td><td>ESP32-C5-MINI-1</td><td>双频 Wi-Fi 6、Bluetooth LE 协处理器，经 SDIO 连接</td></tr>
  <tr><td>存储</td><td>Micro SD 卡槽</td><td>4-bit SDMMC，挂载于 <code>/sdcard</code></td></tr>
  <tr><td>RTC</td><td>PCF85063ATL</td><td>低功耗实时时钟</td></tr>
  <tr><td>CAN</td><td>TJA1051T/3/1J</td><td>高速 CAN 收发器</td></tr>
  <tr><td>RS232</td><td>SP3232EEN</td><td>3.3 V RS-232 收发器</td></tr>
  <tr><td>RS485</td><td>SP3485EN</td><td>3.3 V 半双工 RS-485 收发器</td></tr>
</table>

> 更多硬件信息见[开发板用户指南](../../docs/zh_CN/hw-reference/user-guide-esp32-p4-wifi6-db-touch-lcd-7c.md)。
>
> 板级支持包（BSP）：[BSP 说明](../../components/bsp/esp32_p4_wifi6_db_touch_lcd_7c/README_CN.md) | [API 参考](../../components/bsp/esp32_p4_wifi6_db_touch_lcd_7c/API_CN.md)

## 🛠️ 编译与烧录

需要 ESP-IDF v5.5.x 或 v6.1，并能访问 ESP Component Registry。在本目录下执行：

```sh
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

> [!NOTE]
> 首次烧录必须使用完整 `flash`，以写入分区表、ESP-SR 模型（`srmodels.bin`）和 SPIFFS 资源（`storage.bin`）。之后若只修改了应用代码，可使用 `idf.py -p PORT app-flash monitor` 加快烧录。

### Flash 分区

32 MiB Flash 的分区布局定义在 `partitions.csv`，分区表位于 `0x8000`：

<table>
  <tr><th>名称</th><th>类型</th><th>偏移</th><th>大小</th><th>用途</th></tr>
  <tr><td><code>nvsfactory</code></td><td>data / nvs</td><td><code>0x9000</code></td><td>200 KiB</td><td>出厂 NVS 数据</td></tr>
  <tr><td><code>nvs</code></td><td>data / nvs</td><td><code>0x3B000</code></td><td>840 KiB</td><td>Wi-Fi、应用设置和小智状态</td></tr>
  <tr><td><code>otadata</code></td><td>data / ota</td><td><code>0x10D000</code></td><td>8 KiB</td><td>OTA 数据（保留）</td></tr>
  <tr><td><code>phy_init</code></td><td>data / phy</td><td><code>0x10F000</code></td><td>4 KiB</td><td>PHY 校准数据</td></tr>
  <tr><td><code>model</code></td><td>data / spiffs</td><td><code>0x110000</code></td><td>960 KiB</td><td>ESP-SR 语音模型（<code>srmodels.bin</code>）</td></tr>
  <tr><td><code>factory</code></td><td>app / factory</td><td><code>0x200000</code></td><td>8 MiB</td><td>应用程序</td></tr>
  <tr><td><code>storage</code></td><td>data / spiffs</td><td><code>0xA00000</code></td><td>3 MiB</td><td>音乐、小智提示音和字体（<code>storage.bin</code>）</td></tr>
</table>

## 📱 应用

<table>
  <tr>
    <td align="center"><img src="components/DrawPanel/assets/img_app_drawpanel.png" width="64" alt="DrawPanel 图标"><br><b>DrawPanel</b></td>
    <td align="center"><img src="components/brookesia_app_squareline_demo/assets/esp_brookesia_app_icon_launcher_squareline_112_112.png" width="64" alt="Squareline 图标"><br><b>Squareline</b></td>
    <td align="center"><img src="components/brookesia_app_calculator/assets/img_app_calculator.png" width="64" alt="Calculator 图标"><br><b>Calculator</b></td>
    <td align="center"><img src="components/SpecAnalyzer/assets/img_app_specanalyzer.png" width="64" alt="SpecAnalyzer 图标"><br><b>SpecAnalyzer</b></td>
    <td align="center"><img src="components/MusicPlayer/assets/img_app_musicplayer.png" width="64" alt="MusicPlayer 图标"><br><b>MusicPlayer</b></td>
    <td align="center"><img src="components/Camera/assets/img_app_camera.png" width="64" alt="Camera 图标"><br><b>Camera</b></td>
  </tr>
  <tr>
    <td align="center"><img src="components/VideoPlayer/assets/img_app_vedioplayer.png" width="64" alt="VideoPlayer 图标"><br><b>VideoPlayer</b></td>
    <td align="center"><img src="components/UartViewer/assets/img_app_uart_tool.png" width="64" alt="UART 图标"><br><b>UART</b></td>
    <td align="center"><img src="components/CANViewer/assets/img_app_can_tool.png" width="64" alt="CAN 图标"><br><b>CAN</b></td>
    <td align="center"><img src="components/Settings/assets/img_app_settings.png" width="64" alt="Settings 图标"><br><b>Settings</b></td>
    <td align="center"><img src="components/XiaozhiApp/assets/img_app_xiaozhi.png" width="64" alt="AIChats 图标"><br><b>AIChats</b></td>
  </tr>
</table>

<table>
  <tr><th>应用</th><th>功能</th><th>组件目录</th></tr>
  <tr><td>DrawPanel</td><td>固定画布，在边缘绘制时不会拖动应用</td><td><code>components/DrawPanel/</code></td></tr>
  <tr><td>Squareline</td><td>通过注册表安装的 Squareline Studio UI 示例和应用模板</td><td><code>components/brookesia_app_squareline_demo/</code></td></tr>
  <tr><td>Calculator</td><td>适合触摸操作的 Brookesia 计算器</td><td><code>components/brookesia_app_calculator/</code></td></tr>
  <tr><td>SpecAnalyzer</td><td>板载麦克风的 FFT 频谱</td><td><code>components/SpecAnalyzer/</code></td></tr>
  <tr><td>MusicPlayer</td><td>播放 <code>/spiffs/music</code> 中的本地 MP3，并显示频谱</td><td><code>components/MusicPlayer/</code></td></tr>
  <tr><td>Camera</td><td>OV5647 MIPI CSI 预览，由 PPA 缩放铺满横屏</td><td><code>components/Camera/</code></td></tr>
  <tr><td>VideoPlayer</td><td>全屏播放 SD 卡中的 AVI/MJPEG，音频可用时同步播放声音</td><td><code>components/VideoPlayer/</code></td></tr>
  <tr><td>UART</td><td>RS485/RS232 串口终端，可选波特率、帧格式与 HEX 收发</td><td><code>components/UartViewer/</code></td></tr>
  <tr><td>CAN</td><td>TWAI 终端，125 kbit/s 到 1 Mbit/s，显示总线状态</td><td><code>components/CANViewer/</code></td></tr>
  <tr><td>Settings</td><td>WLAN 扫描/连接、声音、屏幕亮度、SNTP 校时和设备信息</td><td><code>components/Settings/</code></td></tr>
  <tr><td>AIChats</td><td>网络检查、小智激活、WakeNet 唤醒词、VAD、Opus 上下行、服务器文本、表情与 TTS</td><td><code>components/XiaozhiApp/</code></td></tr>
</table>

此外，`components/brookesia_core/` 为本地 ESP-Brookesia core，`components/bsp_extra/` 为基于 BSP 的音频、Wi-Fi 服务和板级外设扩展。

## 🚀 快速体验

1. 上电后等待启动画面完成，进入主屏幕。
2. 打开 **Settings → WLAN**，连接 Wi-Fi，连接成功后会自动校时。
3. 打开 **AIChats**，首次使用会显示并播报激活码；在[小智控制台](https://xiaozhi.me)添加设备并输入激活码。
4. 将 MJPEG 编码的 `.avi` 视频放到 SD 卡的 `avi/` 目录，插卡后在 **VideoPlayer** 中播放。
5. 用 **UART** 和 **CAN** 应用连接外部 RS485/RS232 或 CAN 设备进行收发测试，参数在应用内设置。

## 🎙️ 语音交互

AIChats 激活后进入待机，说出唤醒词 **“你好小智”** 或点击屏幕上的表情即可开始对话。

<table>
  <tr><th>场景</th><th>行为</th></tr>
  <tr><td>小智说话时</td><td>半双工，不会聆听人声；说唤醒词可打断并重新聆听，点击屏幕不会打断</td></tr>
  <tr><td>小智应答时</td><td>屏幕同步显示服务器返回的文字和表情</td></tr>
  <tr><td>一轮应答结束</td><td>自动继续聆听，可连续对话</td></tr>
  <tr><td>服务器结束会话</td><td>回到待机状态，再次说出唤醒词即可开始新对话</td></tr>
  <tr><td>关闭 / 切换应用</td><td>关闭 AIChats 会断开连接，再次打开自动重连；切换到其它应用时保持连接</td></tr>
</table>

> [!NOTE]
> 当前固件尚未注册 MCP 工具，小智暂不能通过语音控制音量、亮度、摄像头等设备功能。

### 切换唤醒词

默认唤醒词为“你好小智”（`wn9l_nihaoxiaozhi_tts3`）。在 `menuconfig` 中切换：

```text
idf.py menuconfig
  ESP Speech Recognition
    Load Multiple Wake Words (WakeNet9)
      [ ] 你好小智 (wn9l_nihaoxiaozhi_tts3)   ← 取消勾选
      [*] Hi,ESP (wn9_hiesp)                  ← 勾选新的唤醒词
```

- 只勾选一个唤醒词：固件只加载找到的第一个 WakeNet 模型。
- ESP32-P4 支持 WakeNet9 / WakeNet9l，不要选择 WakeNet9s 菜单中的模型。
- 唤醒词模型打包在 `srmodels.bin` 中，修改后需重新编译并完整 `flash`，`app-flash` 不会生效。
- 模型需能放进 960 KiB 的 `model` 分区。

### 切换小智显示语言

AIChats 本地界面文字默认为英文，可切换为简体中文：

```text
idf.py menuconfig
  Xiaozhi Application
    Local UI language  (English / Simplified Chinese)
```

修改后重新编译烧录即可，只改应用时可使用 `app-flash`。该选项只影响固件内置的状态文字和激活提示，服务器返回的对话、TTS 和激活信息不受影响；本地字符串位于 `components/XiaozhiApp/XiaozhiLocalization.cpp`。
