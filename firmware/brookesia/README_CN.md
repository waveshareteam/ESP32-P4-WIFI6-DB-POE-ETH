# ESP-Brookesia 出厂固件

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5.4-E7352C?logo=espressif)](https://github.com/espressif/esp-idf/releases/tag/v5.5.4)
[![Target](https://img.shields.io/badge/target-ESP32--P4-1F6FEB)](https://www.espressif.com/zh-hans/products/socs/esp32-p4)
[![LVGL](https://img.shields.io/badge/LVGL-9.4.0-2AA9E0?logo=lvgl)](https://lvgl.io/)

[English](README.md) | 简体中文

本目录是 **Waveshare ESP32-P4-WIFI6-Touch-LCD-4B** 的出厂固件源码工程，并按后续公开维护的目标组织。
固件以 ESP-Brookesia 作为应用框架，为 720 x 720 触摸屏提供应用启动器，
并集成显示、触摸、音频、摄像头、SD 卡、以太网、继电器、RS485 以及
ESP32-C6 Hosted Wi-Fi。

> [!IMPORTANT]
> 这是板卡专用固件，并非通用 ESP-Brookesia 示例。请使用 ESP-IDF
> v5.5.4，并以工程清单中的固定 BSP 提交为准。ESP32-P4 目标、分区表、
> BSP 版本和 ESP32-C6 从机固件共同构成兼容组合，修改后必须进行硬件验证。

## 平台基线

| 层级 | 当前工程基线 |
| --- | --- |
| 主控 | ESP32-P4，32 MB Flash，32 MB PSRAM |
| 显示 / 触摸 | 4 英寸 720 x 720 MIPI DSI ST7703 / GT911 |
| 开发框架 | ESP-IDF v5.5.4，`idf_component.yml` 要求 `>=5.5` |
| UI | LVGL 9.4.0，本地 ESP-Brookesia core 0.6.0-beta2 |
| BSP | `waveshare/esp32_p4_wifi6_touch_lcd_4b` 固定到 `e7731ec5d96a038bf2a1390c9f582dc62a3f36a2` |
| LVGL 适配 | 主界面使用 `espressif/esp_lvgl_adapter` 0.6.4；显示和触摸句柄由 Board Manager 提供 |
| 无线网络 | ESP32-C6 协处理器，通过 SDIO 运行 ESP-Hosted / `esp_wifi_remote` |
| 语音助手 | `espressif/esp_xiaozhi` 0.1.1、ESP-SR 2.4.6、xiaozhi-fonts 1.6.0 |

当前 BSP 来自
[`waveshareteam/Waveshare-ESP32-components#178`](https://github.com/waveshareteam/Waveshare-ESP32-components/pull/178)。
在具备同等功能的正式组件版本发布前，应保留该 Git 提交。固件通过 Board Manager
取得面板和触摸句柄，再由 `esp_lvgl_adapter` 接入 LVGL。VideoPlayer 全屏播放期间使用
adapter 的 Dummy Draw，退出时恢复 LVGL 绘制。

硬件引脚和 P4/C6 兼容关系分别见
[`docs/board.md`](../../docs/board.md) 与
[`docs/p4-c6-hosted-wifi.md`](../../docs/p4-c6-hosted-wifi.md)。

## 应用列表

启动器会安装以下应用。下表直接引用生成 LVGL 资源时使用的源 PNG 图标。

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
    <td align="center"><img src="components/HardwareApps/assets/img_app_ethernet.png" width="64" alt="以太网图标"><br><b>Ethernet</b></td>
    <td align="center"><img src="components/HardwareApps/assets/img_app_relay_control.png" width="64" alt="继电器图标"><br><b>Relay Control</b></td>
    <td align="center"><img src="components/HardwareApps/assets/img_app_rs485.png" width="64" alt="RS485 图标"><br><b>RS485</b></td>
    <td align="center"><img src="components/brookesia_app_squareline_demo/assets/esp_brookesia_app_icon_launcher_squareline_112_112.png" width="64" alt="Squareline 图标"><br><b>Squareline</b></td>
  </tr>
</table>

| 应用 | 功能 | 运行依赖 |
| --- | --- | --- |
| AIChats | 网络检查、小智激活、WakeNet 唤醒、VAD、Opus 上下行、服务端文字与 TTS | Wi-Fi 或以太网、`model`/`storage` 镜像、ES7210/ES8311 |
| Calculator | 适合触摸屏操作的 Brookesia 计算器 | 显示和触摸 |
| DrawPanel | 固定画布区域，边缘绘制不会拖动整个应用 | 显示和触摸 |
| SpecAnalyzer | FC 麦克风的单路 FFT 频谱 | 音频 ADC 通道 0 |
| MusicPlayer | 最多五首本地 MP3，带频谱可视化 | SPIFFS `storage`、ES8311 输出 |
| Settings | WLAN 扫描/连接、声音、屏幕亮度和设备信息子页面 | ESP32-C6 Hosted Wi-Fi 和板级服务 |
| Squareline | 由 Registry 安装的 Squareline Studio UI 演示与应用模板 | 显示和触摸 |
| Camera | PPA 原尺寸居中的 MIPI CSI 预览 | 支持的 OV5647 模式、`esp_video` |
| VideoPlayer | SD 卡全屏 AVI/MJPEG 播放，音频可用时同步播放 | SD 卡、JPEG/PPA、ES8311 输出 |
| Ethernet | IP101 链路、IPv4、掩码、网关、MAC、速率/双工和 PHY 信息 | 板载 IP101 |
| Relay Control | GPIO32、GPIO46 两路高电平有效继电器，状态写入 NVS 并恢复 | 继电器输出和 NVS |
| RS485 | 定时发送、接收窗口、计数与清空功能 | UART1 和板载收发器 |

## 构建

### 环境要求

- 已安装 ESP-IDF v5.5.4 及 `esp32p4` 工具链。
- 首次解析依赖时，可以访问 ESP Component Registry 和固定的 Waveshare
  组件仓库。
- 烧录时准备 ESP32-P4-WIFI6-Touch-LCD-4B 和可传输数据的 USB 线。
- 为托管组件、SPIFFS 镜像和语音模型镜像预留足够磁盘空间。

先激活指定 ESP-IDF 环境，并显式确认版本：

```sh
idf.py --version
idf.py -C firmware/brookesia set-target esp32p4
idf.py -C firmware/brookesia build
```

`sdkconfig.defaults` 已配置 ESP32-P4、32 MB Flash、Hex PSRAM、720 x 720
面板、OV5647、ESP32-C6 Hosted 传输和“你好小智”WakeNet 模型。不要直接修改
`managed_components/` 中的文件；应修改组件清单，再由 IDF Component Manager
重新解析。

工程含本地组件时，Component Manager 生成的 `dependencies.lock` 会记录当前
机器的绝对组件路径，因此该文件只适合本地使用并已忽略。新环境可执行
`idf.py reconfigure` 或首次 `build` 重新生成；可复现的 BSP 提交和组件范围应
维护在各 `idf_component.yml` 中，公开发布物不要包含主机专用锁文件。

### 首次完整烧录与分区变更

首次安装必须同时写入 bootloader、分区表、应用、ESP-SR 模型和 SPIFFS 资源：

```sh
idf.py -C firmware/brookesia -p PORT flash monitor
```

调整分区偏移/大小、从旧分区布局升级，或怀疑模型/文件系统镜像残留时，先整片擦除：

```sh
idf.py -C firmware/brookesia -p PORT erase-flash flash monitor
```

当分区、语音模型和 `spiffs/` 均未变化时，日常代码迭代可只烧应用：

```sh
idf.py -C firmware/brookesia -p PORT app-flash monitor
```

`app-flash` **不会**更新 `srmodels.bin`、`storage.bin` 或分区表。修改
`partitions.csv`、WakeNet/模型 Kconfig、xiaozhi-fonts、内置音乐或提示音后，
必须重新执行完整 `flash`。

## Flash 分区与资源

虽然板卡使用 32 MB Flash，当前分区表有意结束于 16 MiB。这样可以减少完整
烧录的数据量，并在 Quad Flash 32 位寻址未启用时，将 ESP-SR 的 mmap 模型
保留在前 16 MiB。

| 分区 | 偏移 | 大小 | 用途 |
| --- | ---: | ---: | --- |
| `nvsfactory` | `0x009000` | 200 KiB | 出厂/配置 NVS |
| `nvs` | `0x03B000` | 840 KiB | 运行时 NVS，包含应用状态 |
| `otadata` | `0x10D000` | 8 KiB | OTA 选择信息 |
| `phy_init` | `0x10F000` | 4 KiB | PHY 初始化数据 |
| `model` | `0x110000` | 960 KiB | 自动生成的 ESP-SR `srmodels.bin` |
| `factory` | `0x200000` | 12 MiB | 主应用固件 |
| `storage` | `0xE00000` | 2 MiB | 自动生成的 SPIFFS 资源 |

不要把 `model` 简单移动到较大的 `factory` 或 `storage` 后方。若分区需要跨过
16 MiB，必须同步启用并验证本平台适用的 Flash MMAP/32 位地址配置。检测到
32 MB Flash 并不代表所有 mmap 使用方都能读取 16 MiB 以上的地址。

`storage.bin` 从 `spiffs/` 生成，构建时还会把 `78/xiaozhi-fonts` 的 30 px
通用字体放入 `/spiffs/xiaozhi/font.bin`。运行时资源包括：

- `/spiffs/music` 下最多五首由集成方提供的 MusicPlayer MP3；选择可再分发素材和许可证前保持忽略。
- `/spiffs/xiaozhi` 下的数字、激活和成功提示 OGG。
- 小智运行时加载到 PSRAM 的中英文通用字体。

## AIChats / 小智

AIChats 是 `components/XiaozhiApp` 在启动器中的名称。它以托管组件
`esp_xiaozhi` 处理协议，同时保留本板专用的 UI、激活、音频和生命周期适配。

### 首次使用流程

1. 打开 AIChats。应用先显示 UI，再在后台完成自检，不会等待全部网络和音频
   初始化后才显示页面。
2. 应用接受 WLAN Station 或 Ethernet 的有效 IPv4 地址。它不会自行启动配网
   Portal；请在 Settings 中配置 WLAN，或插入网线。
3. 设备尚未绑定小智账号时，屏幕显示并语音播报激活码。进入小智控制面板，
   添加设备并输入该代码。
4. 绑定成功后会播放本地 `success.ogg`，随后连接会话服务。
5. 页面显示待命后，说出已编译的唤醒词（默认“你好小智”），听到首轮回复后
   即可继续对话。

如果日志出现 `No speech model found` 或
`contains no compatible models`，请重新构建并完整烧录，确保分区表和
`srmodels/srmodels.bin` 均写入。仅烧应用无法修复模型分区。

### 编译期 UI 语言

本地状态文字默认使用英文，可通过以下菜单切换：

```text
idf.py menuconfig
  Xiaozhi Application
    Local UI language
      English
      Simplified Chinese
```

对应配置为 `CONFIG_XIAOZHI_APP_UI_LANGUAGE_ENGLISH` 和
`CONFIG_XIAOZHI_APP_UI_LANGUAGE_SIMPLIFIED_CHINESE`。该选项只控制固件自己的
状态文字和激活说明兜底文本；服务端返回的对话文字、TTS 和激活消息会原样
显示或播放，不会因为本地 UI 语言选项而翻译。

如需修改产品默认值，在 `sdkconfig.defaults` 中把 English 配置替换为
Simplified Chinese 配置，并确保 choice 中只有一项为 `y`。已有构建目录会
保留生成的 `sdkconfig`；修改 defaults 后应通过 `menuconfig` 选择，或重新
生成 `sdkconfig`。

新增或修改本地字符串时编辑
`components/XiaozhiApp/XiaozhiLocalization.*`，不要把服务端对话或激活消息
放入本地翻译表。

### 产品工具扩展入口

`components/XiaozhiApp/XiaozhiDeveloperTools.cpp` 中的
`registerXiaozhiDeveloperTools()` 是 MCP 产品工具入口。每次创建 MCP 引擎后、
启动传输前会调用一次；默认实现不注册任何工具。文件中的 TODO 已列出可扩展方向：

- 调节屏幕背光。
- 调节输出音量。
- 拍摄当前摄像头画面，用于视觉请求。

这里只应注册真正可调用的工具：创建 MCP schema 和回调、验证参数、加入引擎，
并返回硬件操作的真实结果。不要让 `tools/list` 暴露尚未实现的占位工具。耗时
硬件操作应投递给应用任务，避免阻塞协议回调；摄像头缓冲区必须复制或持有到
结果构建完成。

### 语音音频约定

AIChats 会把板级音频总线配置为 24 kHz，提取两个麦克风和一路回声参考，
重采样到 16 kHz 后按 `MMR` 顺序交给 ESP-SR。Opus 上行使用 16 kHz；
服务端下行音频在必要时转换为设备的 24 kHz 播放采样率。

| ES7210 串行 TDM 槽位 | 板上实测来源 | 用途 |
| ---: | --- | --- |
| 0 | 物理 MIC1 | SpecAnalyzer 的 FC 频谱；AIChats 麦克风 1 |
| 1 | 物理 MIC3 | AIChats 回声参考 |
| 2 | 物理 MIC2 | AIChats 麦克风 2 |
| 3 | 物理 MIC4 | 保留在完整帧中，当前应用未使用 |

TDM 槽位掩码与 ES7210 物理麦克风增益掩码属于不同命名空间，不能互相代用。
SpecAnalyzer 只分析通道 0 的 FC 输入；AIChats 仍使用 ES7210 的完整四槽 TDM 帧
处理麦克风和回声参考。

## 硬件应用说明

### 继电器状态恢复

Relay Control 在 NVS 命名空间 `relay_control` 的 `output_state` 键中保存两位
状态。应用初始化阶段会在主板重启后恢复 GPIO32/GPIO46；每次打开 UI 时也会读取
当前 GPIO，因此杀掉后台再进入不会把开关显示重置为关闭。主动擦除 NVS 后，两路
输出回到默认关闭状态。

### 以太网

Ethernet 应用启动板载 IP101（PHY 地址 1）和 DHCP，并显示链路、IPv4、掩码、
网关、MAC、速率/双工以及 PHY 信息。引脚属于 `components/bsp_extra` 中的产品
专用适配，详见 [`docs/board.md`](../../docs/board.md)。

### RS485

终端使用 UART1、115200 8N1，TX GPIO47、RX GPIO48。默认开启自动发送，每秒发送：

```text
Hi,this is Waveshare ESP32-P4-WIFI6-Touch-LCD-4B!
```

接收窗口会转义不可打印字节，并显示发送帧数和接收字节数。根据已审核的 V1.3
底板原理图，`/RE` 和 `DE` 相连并被上拉，收发器可能持续处于发送状态，导致接收
路径不可用。不要把 A、B 直接短接做回环，这会破坏差分总线；连接另一台会主动
驱动的 USB-RS485 设备前，应先修正或隔离方向控制。

## 摄像头与媒体要求

### Camera

板级配置选择 OV5647 MIPI RAW8 800 x 800、50 FPS，以及 800 x 1280 RGB565
屏幕。Camera 首次使用时通过 board manager 初始化摄像头，保留设备引用直到重启。
三个 V4L2 MMAP 缓冲经 PPA 按 1:1 原尺寸搬运到两块 PSRAM 图像缓冲，LVGL 图片对象
居中显示，上下各留 240 像素黑边。不缩放、不裁剪；大于屏幕的传感器模式会报错。
摄像头缓冲在图像复制后立即归还，GUI 定时器显示最新完成的帧。

LVGL 和现有触摸输入保持运行，底边上滑返回主页，恢复应用时重新启动采集。
摄像头错误显示在应用界面。DEBUG 日志提供预览帧率，传感器模式的 50 FPS
不代表实测显示帧率。仍需构建和上板验证。

### VideoPlayer

VideoPlayer 挂载 SD 卡后只扫描 `/sdcard/avi`。文件名没有额外要求，`.avi`
扩展名不区分大小写；SD 根目录中的 AVI 和 MP4 都不会被当前逻辑枚举。播放器把
AVI/MJPEG 画面解码为全屏显示，循环播放目录中的文件；音频初始化成功时同步输出
AVI 音频，失败时继续静音播放画面。

播放器使用 `esp_video_player_service` 和 `esp_playlist` 管理 AVI 播放。项目内的
`esp_video_render` backend 将完整视频帧写入 adapter 的三缓冲 Dummy Draw；播放时绕过
LVGL 屏幕绘制，但保留 adapter 输入处理以支持上滑退出。退出后关闭 Dummy Draw 并重绘
LVGL 界面。当前显示配置使用 RGB565；实际解码、画面比例及吞吐量仍需上板验证。

音频输出由 `esp_video_player_service` 管理，并通过 Board Manager 初始化音频 DAC。
SD 卡、DAC 或 service 初始化失败时，应用会显示错误。具体格式支持和音画同步仍需
上板验证。
扫描结果导入 `esp_playlist`，由它管理当前文件和列表循环；
文件完整路径长度必须小于 `CONFIG_ESP_PLAYLIST_URL_MAX`，超长路径会明确报错，
避免被组件截断后打开错误文件。上滑退出。暂停和关闭由播放器 service 完成后再清理
渲染资源。

### MusicPlayer

MusicPlayer 枚举 `/spiffs/music` 中最多五首由集成方提供的 MP3，UI 会复用三套
频谱素材，不再包含专辑封面。示例 MP3 在确定可再分发素材及许可证前保持忽略，因此公开仓库的
干净检出可能没有曲目。发布前应加入已获准分发的音频，或说明集成方如何提供资源。
替换或增加曲目后必须重新生成并烧录 `storage.bin`，只执行 `app-flash` 不会
生效。

## 工程结构

| 路径 | 职责 |
| --- | --- |
| `main/main.cpp` | 板卡启动、Loading UI、Phone Shell、应用安装和状态事件 |
| `components/brookesia_core/` | 本地 Brookesia 应用框架与 Phone UI |
| `components/bsp_extra/` | 音频、背光、继电器、IP101、RS485 的产品专用适配 |
| `components/XiaozhiApp/` | AIChats UI、激活、音频管线、协议适配和 MCP 扩展入口 |
| `components/HardwareApps/` | Relay、Ethernet、RS485 三个硬件应用 |
| `components/<App>/` | 各启动器应用及编译资源 |
| `spiffs/` | 生成 `storage` 分区镜像的源资源 |
| `archive/` | 不参与当前安装的历史实现，仅供参考 |
| `partitions.csv` | 出厂固件分区表 |
| `sdkconfig.defaults` | 维护中的工程默认配置 |
| `managed_components/` | Component Manager 生成目录，不要直接编辑 |
| `build/` | 构建产物，不属于源码 |

## 新增应用

基础生命周期可参考体积较小的 `brookesia_app_calculator`；需要后台任务和共享硬件
时，可参考 `HardwareApps` 或 `XiaozhiApp`。

1. 新建 `components/<AppName>/`，包含 `CMakeLists.txt`、`idf_component.yml`、
   头文件、实现和 `assets/`。
2. 继承 `esp_brookesia::systems::phone::App`，必须实现 `run()` 和 `back()`。
   再根据应用持有的资源按需覆盖 `init()`、`deinit()`、`close()`、`pause()`、
   `resume()` 和 `cleanResource()`。
3. LVGL 对象必须在 LVGL 线程中创建/修改，或仅在很短的显示锁范围内访问。
   后台任务发布数据，再由 timer/async 回调更新 UI。
4. 除编译后的 LVGL 图标外，保留源 PNG，方便审核启动器效果和文档引用。
5. 第三方依赖写入组件清单，不要复制托管组件源码。
6. 在 `main/main.cpp` 中获取单例并调用 `phone->installApp()`，或对适合的应用
   使用现有 Registry 机制。
7. 验证反复打开、切后台、恢复、关闭和重启。释放应用拥有的 timer、task、
   queue、codec session、SD handle 和 LVGL object。

## 常见问题与易踩坑点

- **长时间持有 LVGL 锁会让动画停住。** 显示锁只能包围 UI 操作；网络、SD
  扫描、音频初始化、NVS、模型加载和应用构造必须在锁外进行。
- **应用编译成功不代表数据镜像已更新。** 模型、字体、提示音、音乐和分区变化
  需要完整 `flash`。
- **模型分区位置敏感。** 未在本板启用并验证 Flash MMAP/32 位寻址前，保持
  ESP-SR 模型在 16 MiB 以下。
- **ES7210 名称不是 DMA 槽位号。** 使用实测的
  `[MIC1, MIC3(echo), MIC2, MIC4]` 串行顺序，并区分物理增益掩码与 TDM 掩码。
- **多个音频应用共享同一物理总线。** 在语音、音乐和视频间切换时应停止或移交
  codec session，不要由多个应用独立重配成对的 I2S 通道。
- **摄像头尺寸由传感器模式决定。** 面向屏幕做裁剪/缩放，不要假设 CSI 节点
  接受 LCD 宽高。
- **VideoPlayer 仅支持当前 AVI 路径。** 使用 `/sdcard/avi/*.avi`；当前不支持
  MP4 枚举/播放。
- **P4 本身没有 Wi-Fi。** Host 组件必须与 ESP32-C6 中运行的从机固件兼容；
  只升级 P4 依赖可能出现“能编译但无线不可用”。
- **BSP 固定在未发布提交。** 更新前审核上游 PR/Release，重点确认 Board Manager
  设备句柄和配置结构没有变化。
- **RS485 接收取决于硬件版本。** 软件排障前先检查收发器方向引脚。

## 诊断开关

`main/main.cpp` 中 `EXAMPLE_SHOW_MEM_INFO = false` 默认关闭周期 SRAM/PSRAM
统计。排查内存压力时可临时启用，原有诊断逻辑仍然保留。

反馈问题时，建议提供第一个真实错误、分区/模型加载行、Camera V4L2 格式、
Ethernet 链路事件或四路 ES7210 peak，同时注明板卡版本、ESP-IDF 版本和完整
复现步骤。

## 贡献、支持与发布检查

提交修改前请阅读仓库级[贡献指南](../../CONTRIBUTING.md)、
[支持说明](../../SUPPORT.md)和[安全策略](../../SECURITY.md)。可复用的 BSP/驱动
修复应尽量提交到对应上游组件，本工程只保留产品组合和板级专用适配。

本地 Brookesia core 保留其
[Apache-2.0 许可证](components/brookesia_core/license.txt)。该许可证仅适用于对应
组件，不等于已经为整个仓库选择许可证。

发布前至少完成：

- 在干净源码环境中使用目标 ESP-IDF 版本重新解析依赖并构建。
- 完整烧录并验证显示/触摸、两个麦克风、喇叭、Wi-Fi、以太网、Camera、
  SD/Video、继电器、RS485 硬件限制，以及小智激活和完整唤醒/对话/结束流程。
- 记录兼容的 ESP32-C6 Hosted 从机固件版本。
- 发布完整烧录文件集合，而不是只有应用 `.bin`。
- 删除构建产物，并检查公开文本中不存在本机路径、用户名、凭据、激活标识和
  Wi-Fi 密码。
- 公开分发前选择并添加仓库级 `LICENSE.txt`，同时保留所有组件许可证、SPDX
  头和第三方 Notice。在仓库许可证确定前，不能把任一组件许可证当作整个仓库的
  授权声明。
