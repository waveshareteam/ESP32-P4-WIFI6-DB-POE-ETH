# ESP32-P4-WIFI6-DB-POE-ETH

[English](../../en/hw-reference/user-guide-esp32-p4-wifi6-db-poe-eth.md)

ESP32-P4-WIFI6-DB-POE-ETH 是一款以 ESP32-P4 为主处理器（双核高性能 RISC-V + 单核低功耗核心）、
搭配 ESP32-C5 Wi-Fi 6 / Bluetooth 5（LE）协处理器（通过 SDIO 连接）的开发板。板载 RMII 以太网接口，
并提供 MIPI-DSI 显示接口（可搭配触摸屏）和 MIPI-CSI 摄像头接口。板载已集成 PoE 模块；使用 PoE
供电时，需连接支持 IEEE 802.3af PoE 标准的路由器或交换机。

<!-- 图片占位：docs/_static/esp32-p4-wifi6-db-poe-eth-board.png（开发板实拍图，俯视） -->

## 板载资源

- **主处理器**：ESP32-P4NRW32X，双核高性能 RISC-V + 单核低功耗核。
- **Flash**：32 MB，QIO 模式。
- **PSRAM**：封装内集成 PSRAM，工作频率 200 MHz；容量以对应硬件版本资料为准。
- **无线协处理器**：ESP32-C5，支持 Wi-Fi 6 和 Bluetooth 5 LE，通过 4-bit SDIO 与 ESP32-P4 通信。
- **以太网**：RMII MAC/PHY，PHY 地址为 1，REF_CLK 为 50 MHz 外部输入。
- **显示**：双 lane MIPI-DSI，可搭配 HX8394（5 英寸，720×1280）、ILI9881C（7 英寸，720×1280）或 JD9365（8/10.1 英寸，800×1280）。本页默认按 10.1 英寸 JD9365 屏幕说明。LCD 未接复位引脚，背光通过 I2C 控制。
- **触摸**：GT911 电容触摸控制器，接在共享 I2C 总线上；触摸状态通过轮询读取，复位和中断信号未接。
- **音频**：ES8311 编解码器，扬声器输出 + 单路模拟麦克风输入。
- **存储**：microSD，4-bit SDMMC，带电源使能控制。
- **摄像头**：MIPI-CSI，2-lane，支持 OV5647 和 SC2336 传感器。摄像头所需的时钟和复位信号请按摄像头模组及对应硬件版本确认。
- **USB**：USB 2.0 Host 接口通过 ESP32-P4 内置 USB PHY 的 DM/DP 引脚连接，并带过流保护。
- **扩展排针**：提供多路 GPIO；GPIO45 与 microSD 电源控制共用，GPIO53 与扬声器功放使能共用，GPIO6 用于唤醒 ESP32-C5。详见下文引脚说明。

<!-- 图片占位：docs/_static/esp32-p4-wifi6-db-poe-eth-connectors.png（接口/端口标注图） -->

## 外设速查

| 模块 | 器件 / 功能 | 接口 | 地址 / 参数 | GPIO / 信号 |
| --- | --- | --- | --- | --- |
| LCD | MIPI-DSI 显示屏 | MIPI-DSI | 2-lane；HX8394/ILI9881C/JD9365 | 未接 LCD 复位引脚；背光通过共享 I2C 控制 |
| 触摸 | GT911 电容触摸 | I2C | 7-bit 地址 `0x5D` 或 `0x14`，取决于控制器配置 | SDA=GPIO7，SCL=GPIO8；RST/INT 未接，触摸状态通过轮询读取 |
| LCD 背光 | 背光控制器 | I2C | 7-bit 地址 `0x45` | SDA=GPIO7，SCL=GPIO8 |
| 摄像头 | MIPI-CSI 摄像头接口 | MIPI-CSI + SCCB | 2-lane；支持 OV5647、SC2336，具体地址以模组资料为准 | SCCB 复用 GPIO7/GPIO8；时钟和复位信号按模组及板卡版本确认 |
| 音频 | ES8311 编解码器 | I2C + I2S | I2C 7-bit 地址 `0x18`；单路模拟麦克风 | I2C：SDA=GPIO7，SCL=GPIO8；MCLK=GPIO13，SCLK=GPIO12，WS=GPIO10，DOUT=GPIO9，DIN=GPIO11 |
| 功放控制 | 板载扬声器功放使能 | GPIO | 高电平有效 | GPIO53 |
| TF 卡 | microSD 卡 | SDMMC | 4-bit 模式 | CLK=GPIO43，CMD=GPIO44，D0=GPIO39，D1=GPIO40，D2=GPIO41，D3=GPIO42；电源控制=GPIO45（低有效） |
| 以太网 | RMII PHY | RMII + SMI | PHY 地址 1；50 MHz REF_CLK 外部输入 | CRS_DV=GPIO28，RXD0=GPIO29，RXD1=GPIO30，MDC=GPIO31，TXD0=GPIO34，TXD1=GPIO35，TX_EN=GPIO49，REF_CLK=GPIO50，RESET=GPIO51，MDIO=GPIO52 |
| Wi-Fi/BT 协处理器 | ESP32-C5 | SDIO | 4-bit，40 MHz | CLK=GPIO18，CMD=GPIO19，D0=GPIO14，D1=GPIO15，D2=GPIO16，D3=GPIO17，RESET=GPIO54（低有效） |
| USB | USB Host | USB 2.0 | 芯片内置 USB PHY，支持过流保护 | D+/D- 直连 ESP32-P4 DM/DP |

## 引脚定义

### 扩展排针

扩展排针引出下表所列的 GPIO。GPIO6 同时用于唤醒 ESP32-C5，GPIO45 与 microSD 电源控制共用，GPIO53 与扬声器功放使能共用。使用前请参阅“使用注意”。

| 类型 | 信号 |
| --- | --- |
| GPIO | GPIO2、GPIO3、GPIO4、GPIO5、GPIO6\*、GPIO20、GPIO21、GPIO22、GPIO23、GPIO24、GPIO25、GPIO26、GPIO27、GPIO32、GPIO33、GPIO36、GPIO45\*、GPIO46、GPIO47、GPIO48、GPIO53\* |

\* GPIO6 同时用于唤醒 ESP32-C5；GPIO45 是 microSD 电源使能引脚；GPIO53 是音频功放使能引脚。
对应功能工作期间，请勿单独驱动这些引脚。

<!-- 图片占位：docs/_static/esp32-p4-wifi6-db-poe-eth-pinout.png（完整引脚图） -->

## GPIO 引脚功能

下表按 GPIO 编号列出 ESP32-P4 的信号用途和板载连接。引脚复用及扩展排针位置请结合本板引脚图确认。

| GPIO | 信号名 | 连接到 | 备注 |
| --- | --- | --- | --- |
| GPIO0 | XTAL_32K_N | 32.768 kHz 晶振 | 与普通 GPIO 功能复用；使用前应确认并处理晶振电路 |
| GPIO1 | XTAL_32K_P | 32.768 kHz 晶振 | 与普通 GPIO 功能复用；使用前应确认并处理晶振电路 |
| GPIO2 | GPIO2 | 扩展排针 | 可作普通 GPIO 使用 |
| GPIO3 | GPIO3 | 扩展排针 | 可作普通 GPIO 使用 |
| GPIO4 | GPIO4 | 扩展排针 | 可作普通 GPIO 使用 |
| GPIO5 | GPIO5 | 扩展排针 | 可作普通 GPIO 使用 |
| GPIO6 | GPIO6 | ESP32-C5 唤醒 / 扩展排针 | 用于唤醒 ESP32-C5；接作其他用途前需确认不影响唤醒信号 |
| GPIO7 | I2C SDA | ES8311、GT911、LCD 背光控制器、摄像头 SCCB | 板载共享 I2C 数据线 |
| GPIO8 | I2C SCL | ES8311、GT911、LCD 背光控制器、摄像头 SCCB | 板载共享 I2C 时钟线 |
| GPIO9 | I2S DOUT | ES8311 DSDIN | 音频播放数据，ESP32-P4 → codec |
| GPIO10 | I2S WS | ES8311 LRCK | 音频帧同步 |
| GPIO11 | I2S DIN | ES8311 ASDOUT | 音频采集数据，codec → ESP32-P4 |
| GPIO12 | I2S SCLK | ES8311 SCLK | 音频位时钟 |
| GPIO13 | I2S MCLK | ES8311 MCLK | 音频主时钟 |
| GPIO14 | SDIO D0 | ESP32-C5 | SDIO 数据线 |
| GPIO15 | SDIO D1 | ESP32-C5 | SDIO 数据线 |
| GPIO16 | SDIO D2 | ESP32-C5 | SDIO 数据线 |
| GPIO17 | SDIO D3 | ESP32-C5 | SDIO 数据线 |
| GPIO18 | SDIO CLK | ESP32-C5 | SDIO 时钟线 |
| GPIO19 | SDIO CMD | ESP32-C5 | SDIO 命令线 |
| GPIO20 | GPIO20 | 扩展排针 | 可作普通 GPIO 使用 |
| GPIO21 | GPIO21 | 扩展排针 | 可作普通 GPIO 使用 |
| GPIO22 | GPIO22 | 扩展排针 | 可作普通 GPIO 使用 |
| GPIO23 | GPIO23 | 扩展排针 | 可作普通 GPIO 使用 |
| GPIO24 | GPIO24 | 扩展排针 | 可作普通 GPIO 使用 |
| GPIO25 | GPIO25 | 扩展排针 | 可作普通 GPIO 使用 |
| GPIO26 | GPIO26 | 扩展排针 | 可作普通 GPIO 使用 |
| GPIO27 | GPIO27 | 扩展排针 | 可作普通 GPIO 使用 |
| GPIO28 | RMII CRS_DV | 以太网 PHY | 载波侦听/接收数据有效，PHY → ESP32-P4（输入） |
| GPIO29 | RMII RXD0 | 以太网 PHY | 接收数据位 0，PHY → ESP32-P4（输入） |
| GPIO30 | RMII RXD1 | 以太网 PHY | 接收数据位 1，PHY → ESP32-P4（输入） |
| GPIO31 | SMI MDC | 以太网 PHY | PHY 管理接口时钟，ESP32-P4 → PHY（输出） |
| GPIO32 | GPIO32 | 扩展排针 | 可作普通 GPIO 使用 |
| GPIO33 | GPIO33 | 扩展排针 | 可作普通 GPIO 使用 |
| GPIO34 | RMII TXD0 | 以太网 PHY | 发送数据位 0，ESP32-P4 → PHY（输出）；上电/复位时也是启动配置采样引脚 |
| GPIO35 | RMII TXD1 | 以太网 PHY | 发送数据位 1，ESP32-P4 → PHY（输出）；上电/复位时也是启动配置采样引脚 |
| GPIO36 | GPIO36 | 扩展排针 | 可作普通 GPIO 使用；上电/复位时也是启动配置采样引脚 |
| GPIO37 | UART0_TXD | 下载/调试串口 | UART0 发送信号，用于烧录和串口调试；上电/复位时也是启动配置采样引脚 |
| GPIO38 | UART0_RXD | 下载/调试串口 | UART0 接收信号，用于烧录和串口调试；上电/复位时也是启动配置采样引脚 |
| GPIO39 | SD_D0 | microSD 卡 | SDMMC 4-bit 数据线 |
| GPIO40 | SD_D1 | microSD 卡 | SDMMC 4-bit 数据线 |
| GPIO41 | SD_D2 | microSD 卡 | SDMMC 4-bit 数据线 |
| GPIO42 | SD_D3 | microSD 卡 | SDMMC 4-bit 数据线 |
| GPIO43 | SD_CLK | microSD 卡 | SDMMC 时钟线 |
| GPIO44 | SD_CMD | microSD 卡 | SDMMC 命令线 |
| GPIO45 | SD_VDD_EN | microSD 电源控制 / 扩展排针 | 低电平有效；SD 卡工作期间不建议作为普通 GPIO 使用 |
| GPIO46 | GPIO46 | 扩展排针 | 可作普通 GPIO 使用 |
| GPIO47 | GPIO47 | 扩展排针 | 可作普通 GPIO 使用 |
| GPIO48 | GPIO48 | 扩展排针 | 可作普通 GPIO 使用 |
| GPIO49 | RMII TX_EN | 以太网 PHY | 发送使能，ESP32-P4 → PHY（输出） |
| GPIO50 | RMII REF_CLK | 以太网 PHY | 50 MHz RMII 参考时钟，PHY → ESP32-P4（输入） |
| GPIO51 | PHY RESET | 以太网 PHY | PHY 硬件复位控制，ESP32-P4 → PHY（GPIO 输出） |
| GPIO52 | SMI MDIO | 以太网 PHY | PHY 管理接口数据，双向 |
| GPIO53 | PA_CTRL | 扬声器功放使能 / 扩展排针 | 高电平有效；音频播放期间不建议作为普通 GPIO 使用 |
| GPIO54 | C5_RESET | ESP32-C5 复位 | 低电平有效，复位 Wi-Fi/BT 协处理器 |

## 使用注意

- GPIO7/GPIO8 为板载共享 I2C 总线，接入 ES8311、GT911、LCD 背光控制器和摄像头 SCCB 接口，
  外接其他 I2C 设备时需确认地址不冲突。
- GPIO45 控制 microSD 电源，GPIO53 控制音频功放，GPIO54 用于复位 ESP32-C5，GPIO6 用于唤醒
  ESP32-C5。对应功能工作时，不要同时将这些引脚用于其他用途。
- GPIO28～GPIO31、GPIO34～GPIO35、GPIO49～GPIO52 用于以太网 RMII/SMI 信号和 PHY 复位，
  不建议作为普通 GPIO 使用。
- GPIO34～GPIO38 是 ESP32-P4 的启动配置采样引脚，在上电或复位期间由芯片读取。GPIO34/GPIO35
  同时用于以太网 TXD0/TXD1，GPIO37/GPIO38 同时用于 UART0 收发（常用于烧录和串口日志）。复位释放后，
  这些引脚可承担各自的其他功能；外接电路不得在启动采样期间干扰其电平。
- GPIO0/GPIO1 分别接板上 32.768 kHz 晶振的 XTAL_32K_N/XTAL_32K_P，供 ESP32-P4 RTC 慢时钟
  使用。若要将这两个引脚用于其他用途，应先确认并处理晶振电路，避免信号冲突。
- LCD 复位、GT911 复位和中断信号未接线；触摸状态通过 I2C 轮询读取。
- 扩展排针的引脚位置和 GPIO 编号请对照本板引脚图确认。

## 产品尺寸

<!-- 图片占位：docs/_static/esp32-p4-wifi6-db-poe-eth-dimensions.png（结构尺寸图） -->

产品尺寸信息待补充。
