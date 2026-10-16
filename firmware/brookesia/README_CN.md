# ESP-Brookesia 出厂固件

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5.4-E7352C?logo=espressif)](https://github.com/espressif/esp-idf/releases/tag/v5.5.4)
[![Target](https://img.shields.io/badge/target-ESP32--P4-1F6FEB)](https://www.espressif.com/zh-hans/products/socs/esp32-p4)
[![LVGL](https://img.shields.io/badge/LVGL-9.4.0-2AA9E0?logo=lvgl)](https://lvgl.io/)

[English](README.md) | 简体中文

本目录包含 **Waveshare ESP32-P4-WIFI6-DB-POE-ETH** 的 ESP-Brookesia 固件源码，
支持显示、触摸、音频、摄像头、SD 卡和 ESP32-C5 Hosted Wi-Fi。板卡配置中同时
包含 IP101 RMII 以太网设备。

## 平台配置

| 项目 | 配置 |
| --- | --- |
| 主控 | ESP32-P4NRW32X，32 MB Flash，32 MB PSRAM |
| 显示 / 触摸 | 支持 Waveshare [5](https://www.waveshare.net/shop/5-DSI-TOUCH-A.htm)、[7](https://www.waveshare.net/shop/7-DSI-TOUCH-A.htm)、[8](https://www.waveshare.net/shop/8-DSI-TOUCH-A.htm)、[10.1](https://www.waveshare.net/shop/10.1-DSI-TOUCH-A.htm) 英寸面板 |
| 开发框架 | ESP-IDF v5.5.4，`idf_component.yml` 要求 `>=5.5` |
| UI | LVGL 9.4.0，本地 ESP-Brookesia core 0.6.0-beta2 |
| Board Manager | `esp32_p4_wifi6_db_poe_eth`，`espressif/esp_board_manager` 0.7.4 |
| LVGL 适配 | `espressif/esp_lvgl_adapter` 0.6.4 |
| 无线网络 | ESP32-C5，通过 SDIO slot 1 运行 ESP-Hosted / `esp_wifi_remote` |
| 语音助手 | `espressif/esp_xiaozhi` 0.1.1、ESP-SR 2.4.6、xiaozhi-fonts 1.6.0 |

显示、触摸、ES8311、SD 卡、摄像头、背光和 IP101 由 Board Manager 板卡定义提供。
硬件接口与引脚见[硬件参考指南](../../docs/zh_CN/hw-reference/user-guide-esp32-p4-wifi6-db-poe-eth.md)，
ESP32-P4/ESP32-C5 配套工程见
[`15_wifi_iperf`](../../examples/esp-idf/15_wifi_iperf/)。

## 应用列表

<table>
  <tr>
    <td align="center"><img src="components/XiaozhiApp/assets/img_app_xiaozhi.png" width="64" alt="AIChats 图标"><br><b>AIChats</b></td>
    <td align="center"><img src="components/brookesia_app_calculator/assets/img_app_calculator.png" width="64" alt="计算器图标"><br><b>Calculator</b></td>
    <td align="center"><img src="components/draw/assets/img_app_drawpanel.png" width="64" alt="画板图标"><br><b>DrawPanel</b></td>
    <td align="center"><img src="components/SpecAnalyzer/assets/img_app_specanalyzer.png" width="64" alt="频谱图标"><br><b>SpecAnalyzer</b></td>
    <td align="center"><img src="components/MusicPlayer/assets/img_app_musicplayer.png" width="64" alt="音乐播放器图标"><br><b>MusicPlayer</b></td>
    <td align="center"><img src="components/Settings/assets/img_app_settings.png" width="64" alt="设置图标"><br><b>Settings</b></td>
  </tr>
  <tr>
    <td align="center"><img src="components/Camera/assets/img_app_camera.png" width="64" alt="摄像头图标"><br><b>Camera</b></td>
    <td align="center"><img src="components/VideoPlayer/assets/img_app_vedioplayer.png" width="64" alt="视频播放器图标"><br><b>VideoPlayer</b></td>
    <td align="center"><img src="components/brookesia_app_squareline_demo/assets/esp_brookesia_app_icon_launcher_squareline_112_112.png" width="64" alt="Squareline 图标"><br><b>Squareline</b></td>
  </tr>
</table>

| 应用 | 功能 | 运行依赖 |
| --- | --- | --- |
| AIChats | 网络检查、小智激活、WakeNet 唤醒、VAD、Opus 上下行、服务端文字与 TTS | Wi-Fi、`model`/`storage` 镜像、ES8311 |
| Calculator | 适合触摸屏操作的 Brookesia 计算器 | 显示和触摸 |
| DrawPanel | 固定画布区域，边缘绘制不会拖动整个应用 | 显示和触摸 |
| SpecAnalyzer | FC 麦克风的单路 FFT 频谱 | 音频 ADC 通道 0 |
| MusicPlayer | 最多五首本地 MP3，带频谱可视化 | SPIFFS `storage`、ES8311 输出 |
| Settings | WLAN 扫描/连接、声音、屏幕亮度和设备信息 | ESP32-C5 Hosted Wi-Fi 和板级服务 |
| Squareline | Registry 安装的 Squareline Studio UI 演示与应用模板 | 显示和触摸 |
| Camera | PPA 等比例缩放并居中的 MIPI CSI 预览 | OV5647、`esp_video` |
| VideoPlayer | SD 卡全屏 AVI/MJPEG 播放，音频可用时同步播放 | SD 卡、JPEG/PPA、ES8311 输出 |

## 构建

### 环境要求

需要 ESP-IDF v5.5.4、`esp32p4` 工具链，以及访问 ESP Component Registry 和
Waveshare 组件仓库的网络连接。烧录时请使用支持数据传输的 USB 线。

```sh
idf.py --version
idf.py -C firmware/brookesia set-target esp32p4
idf.py -C firmware/brookesia build
```

### 首次完整烧录与分区变更

首次安装需要写入 bootloader、分区表、应用、ESP-SR 模型和 SPIFFS 资源：

```sh
idf.py -C firmware/brookesia -p PORT flash monitor
```

修改分区偏移或大小、从旧分区布局升级，或者怀疑模型和文件系统镜像残留时，
先擦除 Flash 再完整烧录：

```sh
idf.py -C firmware/brookesia -p PORT erase-flash flash monitor
```

分区、语音模型和 `spiffs/` 资源均未变化时，可以只烧录应用：

```sh
idf.py -C firmware/brookesia -p PORT app-flash monitor
```

`app-flash` 不会更新 `srmodels.bin`、`storage.bin` 或分区表。
修改 `partitions.csv`、WakeNet/模型 Kconfig、xiaozhi-fonts、内置音乐或提示音后，
必须重新执行完整 `flash`。

## AIChats / 小智

AIChats 使用 `esp_xiaozhi` 处理协议，本工程负责 UI、激活、音频和生命周期适配。

首次使用：

1. 在 Settings 中配置 WLAN，或连接以太网。
2. 打开 AIChats；设备未绑定账号时会显示并播报激活码。
3. 在小智控制面板添加设备并输入激活码。
4. 页面进入待命状态后，说出唤醒词，默认是“你好小智”。

如果日志出现 `No speech model found` 或 `contains no compatible models`，请重新构建并
完整烧录，确保分区表和 `srmodels/srmodels.bin` 已写入设备。

本地 UI 语言可在以下菜单中设置：

```text
idf.py menuconfig
  Xiaozhi Application
    Local UI language
      English
      Simplified Chinese
```

该选项只控制固件内置文字，不会翻译服务端返回的对话、TTS 或激活消息。本地字符串位于
`components/XiaozhiApp/XiaozhiLocalization.*`。

板级音频使用 ES8311，I2S 总线为 48 kHz、16-bit、双通道 `FC,RE`。当前硬件只有
一路有效麦克风，AFE 使用 `MN` 布局：`FC` 作为麦克风输入，第二个 I2S 槽位忽略，
不作为 AEC 回声参考。AIChats 将采集数据转换为 16 kHz 单声道 Opus 上行；服务端
下行音频由播放链路按需转换到 48 kHz 输出。SpecAnalyzer 同样只分析 `FC` 通道。

## 板级硬件说明

### 以太网

Board Manager 配置包含板载 IP101（PHY 地址 1）的 RMII 设备定义。当前启动器没有
安装独立的 Ethernet、Relay Control 或 RS485 应用。

## 摄像头与媒体

Camera 使用 OV5647 MIPI RAW8 800 x 800、50 FPS 模式。三个 V4L2 MMAP 缓冲经
PPA 按显示区域等比例缩放到两块 PSRAM 图像缓冲，再由 LVGL 居中显示，不裁剪画面。
DEBUG 日志中的显示帧率需要以上板结果为准。

VideoPlayer 只扫描 `/sdcard/avi`，`.avi` 扩展名不区分大小写。SD 卡根目录中的 AVI
和 MP4 不会被枚举。播放器通过 `avi_player` 读取 AVI，使用 `esp_new_jpeg` 解码 MJPEG，
再由 PPA 缩放后直接写入 LCD 帧缓冲；音频初始化失败时会继续静音播放画面。

MusicPlayer 最多读取 `/spiffs/music` 中的五首 MP3。修改曲目后必须重新生成并烧录
`storage.bin`，只执行 `app-flash` 不会生效。

## 开发说明

- MCP 工具入口是 `components/XiaozhiApp/XiaozhiDeveloperTools.cpp` 中的
  `registerXiaozhiDeveloperTools()`。只注册已经实现并能返回真实结果的工具，耗时操作应交给应用任务处理。
- 新应用继承 `esp_brookesia::systems::phone::App`，实现 `run()` 和 `back()`，再按资源所有权覆盖其他生命周期函数。
- LVGL 对象只能在 LVGL 线程或短时间持有显示锁时操作；关闭应用时应释放其 timer、task、queue、codec session、SD handle 和 LVGL object。
- VideoPlayer 播放时启用 adapter 的 Dummy Draw 并暂停 LVGL，退出后停止 `avi_player`，再恢复 LVGL 绘制。

## 工程结构

| 路径 | 职责 |
| --- | --- |
| `CMakeLists.txt` | ESP-IDF 工程入口 |
| `main/main.cpp` | 初始化 Phone、安装应用并处理 Wi-Fi 状态事件 |
| `components/brookesia_core/` | 本地 Brookesia core 实现 |
| `components/esp_hal_boards/` | Brookesia 音频和显示 HAL 适配 |
| `components/gen_bmgr_codes/` | 选择并生成 Board Manager 板级配置代码 |
| `components/{brookesia_app_calculator, brookesia_app_squareline_demo, draw, Camera, XiaozhiApp, VideoPlayer, MusicPlayer, SpecAnalyzer, Settings}/` | 启动器中的计算器、Squareline 演示、绘图、相机、小智、视频、音乐、频谱和设置应用 |
| `spiffs/` | 生成 `storage` 分区镜像的音乐和小智资源 |
| `partitions.csv` | Flash 分区布局 |
| `sdkconfig.defaults` | 工程默认配置 |

## 注意事项

- LVGL 锁只用于 UI 操作；网络、SD 扫描、音频初始化、NVS 和模型加载应在锁外执行。
- 模型、字体、提示音、音乐和分区变化需要完整 `flash`。
- 多个音频应用共享同一物理总线，切换应用时需要停止或移交 codec session。
- Host 组件必须与 ESP32-C5 中的协处理器固件兼容。
- 更新 BSP 前应检查上游 PR/Release 中的 Board Manager 接口变化。

`main/main.cpp` 中的 `EXAMPLE_SHOW_MEM_INFO = true`，因此会定期统计 SRAM/PSRAM。
详细统计日志由 `EXAMPLE_SHOW_MEM_INFO_LOG` 单独控制。

反馈问题时，请附上板卡版本、ESP-IDF 版本、完整复现步骤和日志中的第一个实际错误。

本地 Brookesia core 使用
[Apache-2.0 许可证](components/brookesia_core/license.txt)，该许可证只适用于对应组件。
公开分发前仍需确定仓库级许可证，并保留组件许可证、SPDX 头和第三方 Notice。
