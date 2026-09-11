# ESP32-P4-WIFI6-DB-POE-ETH

[English](../../en/hw-reference/user-guide-esp32-p4-wifi6-db-poe-eth.md)

ESP32-P4-WIFI6-DB-POE-ETH 是一款以 ESP32-P4 为主处理器（双核高性能 RISC-V + 单核低功耗）、
搭配 ESP32-C5 Wi-Fi 6 / Bluetooth 5（LE）协处理器（通过 SDIO 连接）的开发板。
板载 RMII 以太网并支持 PoE 供电输入，可选配 MIPI-DSI 触摸屏，并提供 MIPI-CSI 摄像头接口。

<!-- 图片占位：docs/_static/esp32-p4-wifi6-db-poe-eth-board.png（开发板实拍图，俯视） -->

> **说明**
>
> 本页内容整理自板级支持包（BSP）源码
> [`components/esp32_p4_wifi6_db_poe_eth`](../../../../components/esp32_p4_wifi6_db_poe_eth/)，
> 反映的是 BSP 实际配置和驱动的内容。软件未涉及的机械/外观信息（接口具体型号、丝印标注、
> 指示灯、板卡尺寸、PHY/电源芯片具体型号等）在本仓库中尚未有原理图可核实，下文以占位形式保留。

## 板载资源

以下内容基于 BSP 的能力标志位（capability flags）和引脚配置确认：

- **主处理器**：ESP32-P4，双核高性能 RISC-V + 单核低功耗核。
- **Flash**：32 MB（QIO）。注意：仓库内示例工程的 `sdkconfig.defaults` 目前配置的是
  `CONFIG_ESPTOOLPY_FLASHSIZE_16MB`，低于板上实际容量，如需使用完整 32 MB 空间请自行调整。
- **PSRAM**：片上 PSRAM，200 MHz（`CONFIG_SPIRAM_SPEED_200M`）；具体容量 BSP 未声明。
- **无线协处理器**：ESP32-C5（Wi-Fi 6 + Bluetooth 5 LE），通过 4-bit SDIO（slot 1）连接，
  由应用层 `esp_hosted` / `esp_wifi_remote` 组件驱动。
- **以太网**：RMII MAC/PHY，使用通用 IEEE 802.3 PHY 驱动（`esp_eth_phy_new_generic`），
  支持 PoE 供电输入。
- **显示**：双 lane MIPI-DSI，屏幕型号通过 Kconfig 选择 —— HX8394（5 英寸，720×1280）、
  ILI9881C（7 英寸，720×1280）、JD9365（8/10.1 英寸，800×1280，默认）。无硬件 LCD 复位引脚，
  背光通过 I2C 控制。
- **触摸**：GT911 电容触摸控制器，接在共享 I2C 总线上，采用轮询方式（复位/中断引脚未接）。
- **音频**：ES8311 编解码器，扬声器输出 + 单路模拟麦克风输入。
- **存储**：microSD，4-bit SDMMC，带电源使能控制。
- **摄像头**：MIPI-CSI，2-lane，通过 `esp_video` 使用，支持 OV5647 和 SC2336 传感器。
  摄像头 XCLK/RESET 未由 BSP 驱动（`GPIO_NUM_NC`）。
- **USB**：通过 ESP32-P4 芯片内置 USB PHY 的 DM/DP 引脚直连，支持过流保护，作为 USB Host
  使用（`usb/usb_host.h`）。
- **扩展排针**：GPIO 排针，由 `bsp_get_header_gpios()` 枚举（见下文）。BSP 未配置 RTC，
  也未配置用户按键/指示灯（`BSP_CAPS_RTC = 0`，`BSP_CAPS_BUTTONS = 0`）。
- **ESP-IDF target**：`esp32p4`，要求 ESP-IDF >= 5.5。

<!-- 图片占位：docs/_static/esp32-p4-wifi6-db-poe-eth-connectors.png（接口/端口标注图） -->

## 外设速查

| 模块 | 器件 / 功能 | 接口 | 地址 / 参数 | GPIO / 信号 |
| --- | --- | --- | --- | --- |
| LCD | MIPI-DSI 显示屏 | MIPI-DSI | 2-lane；HX8394/ILI9881C/JD9365，Kconfig 可选 | 无 LCD 复位引脚（`GPIO_NUM_NC`）；背光通过共享 I2C 控制 |
| 触摸 | GT911 电容触摸 | I2C | 7-bit 地址 `0x5D` 或 `0x14`（均会尝试） | SDA=GPIO7，SCL=GPIO8；RST/INT 未接 |
| LCD 背光 | 背光控制器 | I2C | 7-bit 地址 `0x45`；亮度寄存器 `0x96` | SDA=GPIO7，SCL=GPIO8 |
| 摄像头 | MIPI-CSI 摄像头接口 | MIPI-CSI + SCCB | 2-lane；传感器型号/地址以模块资料为准（支持 OV5647、SC2336） | SCCB 复用 GPIO7/GPIO8；XCLK/RESET 未由 BSP 驱动 |
| 音频 | ES8311 Codec | I2C + I2S | I2C 7-bit 地址 `0x18`（`ES8311_CODEC_DEFAULT_ADDR`），8-bit 写地址 `0x30`；单路模拟麦克风 | I2C：SDA=GPIO7，SCL=GPIO8；MCLK=GPIO13，SCLK=GPIO12，WS=GPIO10，DOUT=GPIO9，DIN=GPIO11 |
| 功放控制 | 板载扬声器功放使能 | GPIO | 高电平有效 | GPIO53 |
| TF 卡 | SDMMC 4-bit | SDMMC | 4-bit 模式，slot 0 | CLK=GPIO43，CMD=GPIO44，D0=GPIO39，D1=GPIO40，D2=GPIO41，D3=GPIO42；电源控制=GPIO45（低有效） |
| Ethernet | RMII PHY（通用 IEEE 802.3 驱动） | RMII + SMI | PHY 地址 1；50 MHz REF_CLK 外部输入 | CRS_DV=GPIO28，RXD0=GPIO29，RXD1=GPIO30，MDC=GPIO31，TXD0=GPIO34，TXD1=GPIO35，TX_EN=GPIO49，REF_CLK=GPIO50，RESET=GPIO51，MDIO=GPIO52 |
| Wi-Fi/BT 协处理器 | ESP32-C5，ESP-Hosted 链路 | SDIO | 4-bit，slot 1，40 MHz | CLK=GPIO18，CMD=GPIO19，D0=GPIO14，D1=GPIO15，D2=GPIO16，D3=GPIO17，RESET=GPIO54（低有效） |
| USB | USB Host | USB 2.0 | 芯片内置 USB PHY，支持过流保护 | D+/D- 直连 ESP32-P4 DM/DP |

## 引脚定义

### 扩展排针

`bsp_get_header_gpios()` 返回下表所示的 BSP 只读 GPIO 数组，其中两个引脚与板载外设复用，
详见"使用注意"。

| 类型 | 信号 |
| --- | --- |
| GPIO | GPIO2、GPIO3、GPIO4、GPIO5、GPIO20、GPIO21、GPIO22、GPIO23、GPIO24、GPIO25、GPIO26、GPIO27、GPIO32、GPIO33、GPIO36、GPIO45\*、GPIO46、GPIO47、GPIO48、GPIO53\* |

\* GPIO45 同时是 microSD 电源使能引脚；GPIO53 同时是音频功放使能引脚。对应外设工作期间，
请勿单独驱动这两个引脚。

GPIO6 同样引到了扩展排针上，但未纳入 `bsp_get_header_gpios()` 的软件枚举；该引脚同时是
ESP32-C5 的唤醒引脚，用作普通 GPIO 前请确认不会影响 ESP32-C5 的唤醒时序。

<!-- 图片占位：docs/_static/esp32-p4-wifi6-db-poe-eth-pinout.png（完整引脚图） -->

## GPIO 完整分配

下表列出全部 GPIO（0～54）及 BSP 源码中的配置情况。"BSP 未使用" 表示该引脚未在
`components/esp32_p4_wifi6_db_poe_eth` 中被引用，其物理走线（如有）在本仓库中尚未核实，
请以后续补充的硬件原理图为准。

| GPIO | 信号名 | 连接到 | 备注 |
| --- | --- | --- | --- |
| GPIO0 | XTAL_32K_N | 32.768 kHz 晶振 | BSP 未配置；若需作为普通 GPIO 使用，需先禁用板上的 XTAL_32K 功能（对应晶振/匹配电阻） |
| GPIO1 | XTAL_32K_P | 32.768 kHz 晶振 | BSP 未配置；若需作为普通 GPIO 使用，需先禁用板上的 XTAL_32K 功能（对应晶振/匹配电阻） |
| GPIO2 | GPIO2 | 扩展排针 | `bsp_get_header_gpios()` |
| GPIO3 | GPIO3 | 扩展排针 | `bsp_get_header_gpios()` |
| GPIO4 | GPIO4 | 扩展排针 | `bsp_get_header_gpios()` |
| GPIO5 | GPIO5 | 扩展排针 | `bsp_get_header_gpios()` |
| GPIO6 | GPIO6 | ESP32-C5 唤醒 / 扩展排针 | 用于唤醒 ESP32-C5；未纳入 `bsp_get_header_gpios()` 软件枚举 |
| GPIO7 | I2C SDA | ES8311、GT911、LCD 背光控制器、摄像头 SCCB | 共享 I2C 数据线；未纳入扩展排针 GPIO 数组 |
| GPIO8 | I2C SCL | ES8311、GT911、LCD 背光控制器、摄像头 SCCB | 共享 I2C 时钟线；未纳入扩展排针 GPIO 数组 |
| GPIO9 | I2S DOUT | ES8311 DSDIN | 音频播放数据，ESP32-P4 → codec |
| GPIO10 | I2S WS | ES8311 LRCK | 音频帧同步 |
| GPIO11 | I2S DIN | ES8311 ASDOUT | 音频采集数据，codec → ESP32-P4 |
| GPIO12 | I2S SCLK | ES8311 SCLK | 音频位时钟 |
| GPIO13 | I2S MCLK | ES8311 MCLK | 音频主时钟 |
| GPIO14 | SDIO D0 | ESP32-C5 | Hosted Wi-Fi/BT 链路 |
| GPIO15 | SDIO D1 | ESP32-C5 | Hosted Wi-Fi/BT 链路 |
| GPIO16 | SDIO D2 | ESP32-C5 | Hosted Wi-Fi/BT 链路 |
| GPIO17 | SDIO D3 | ESP32-C5 | Hosted Wi-Fi/BT 链路 |
| GPIO18 | SDIO CLK | ESP32-C5 | Hosted Wi-Fi/BT 链路 |
| GPIO19 | SDIO CMD | ESP32-C5 | Hosted Wi-Fi/BT 链路 |
| GPIO20 | GPIO20 | 扩展排针 | `bsp_get_header_gpios()` |
| GPIO21 | GPIO21 | 扩展排针 | `bsp_get_header_gpios()` |
| GPIO22 | GPIO22 | 扩展排针 | `bsp_get_header_gpios()` |
| GPIO23 | GPIO23 | 扩展排针 | `bsp_get_header_gpios()` |
| GPIO24 | GPIO24 | 扩展排针 | `bsp_get_header_gpios()` |
| GPIO25 | GPIO25 | 扩展排针 | `bsp_get_header_gpios()` |
| GPIO26 | GPIO26 | 扩展排针 | `bsp_get_header_gpios()` |
| GPIO27 | GPIO27 | 扩展排针 | `bsp_get_header_gpios()` |
| GPIO28 | RMII CRS_DV | 以太网 PHY | 载波侦听/接收数据有效，PHY → ESP32-P4（输入） |
| GPIO29 | RMII RXD0 | 以太网 PHY | 接收数据位 0，PHY → ESP32-P4（输入） |
| GPIO30 | RMII RXD1 | 以太网 PHY | 接收数据位 1，PHY → ESP32-P4（输入） |
| GPIO31 | SMI MDC | 以太网 PHY | PHY 管理接口时钟，ESP32-P4 → PHY（输出） |
| GPIO32 | GPIO32 | 扩展排针 | `bsp_get_header_gpios()` |
| GPIO33 | GPIO33 | 扩展排针 | `bsp_get_header_gpios()` |
| GPIO34 | RMII TXD0 | 以太网 PHY | 发送数据位 0，ESP32-P4 → PHY（输出） |
| GPIO35 | RMII TXD1 | 以太网 PHY | 发送数据位 1，ESP32-P4 → PHY（输出）；ESP32-P4 strapping 引脚，见"使用注意" |
| GPIO36 | GPIO36 | 扩展排针 | `bsp_get_header_gpios()`；ESP32-P4 strapping 引脚，见"使用注意" |
| GPIO37 | UART0_TXD | 下载/调试串口 | 用于烧录和串口监视器；不建议作为普通 GPIO 使用 |
| GPIO38 | UART0_RXD | 下载/调试串口 | 用于烧录和串口监视器；不建议作为普通 GPIO 使用 |
| GPIO39 | SD_D0 | microSD 卡 | SDMMC 4-bit 数据线 |
| GPIO40 | SD_D1 | microSD 卡 | SDMMC 4-bit 数据线 |
| GPIO41 | SD_D2 | microSD 卡 | SDMMC 4-bit 数据线 |
| GPIO42 | SD_D3 | microSD 卡 | SDMMC 4-bit 数据线 |
| GPIO43 | SD_CLK | microSD 卡 | SDMMC 时钟线 |
| GPIO44 | SD_CMD | microSD 卡 | SDMMC 命令线 |
| GPIO45 | SD_VDD_EN | microSD 电源控制 / 扩展排针 | 低电平有效；SD 卡工作期间不建议作为普通 GPIO 使用 |
| GPIO46 | GPIO46 | 扩展排针 | `bsp_get_header_gpios()` |
| GPIO47 | GPIO47 | 扩展排针 | `bsp_get_header_gpios()` |
| GPIO48 | GPIO48 | 扩展排针 | `bsp_get_header_gpios()` |
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
  ESP32-C5；对应外设工作时不建议单独驱动这些引脚。
- GPIO37/GPIO38 是 UART0 下载串口（TXD/RXD），用于烧录固件和串口监视器，不建议作为普通
  GPIO 使用。
- GPIO28～GPIO31、GPIO34～GPIO35、GPIO49～GPIO52 用于以太网 RMII/SMI 信号和 PHY 复位，
  不建议作为普通 GPIO 使用。
- GPIO34～GPIO37 是 ESP32-P4 芯片级的 strapping 引脚；本板 GPIO34/GPIO35 已用于以太网
  TXD0/TXD1。在上电或复位阶段改变这些引脚电平前，请先查阅 ESP32-P4 技术规格书确认具体定义。
- GPIO0/GPIO1 分别接板上 32.768 kHz 晶振的 XTAL_32K_N/XTAL_32K_P，供 ESP32-P4 RTC 慢时钟
  使用。BSP 当前未配置该功能；若要将这两个引脚挪作普通 GPIO，需要先禁用晶振电路，避免与
  晶振信号冲突。
- LCD 复位、GT911 复位和中断信号均未接线，由显示/触摸驱动通过软件流程处理（触摸采用轮询）。
- 扩展排针 GPIO 数组（`bsp_get_header_gpios()`）是软件层面的枚举，供 `12_generic_gpio`
  等示例使用；不代表排针连接器上实际引出的全部引脚。

## 产品尺寸

<!-- 图片占位：docs/_static/esp32-p4-wifi6-db-poe-eth-dimensions.png（结构尺寸图） -->

硬件文档待补充。
