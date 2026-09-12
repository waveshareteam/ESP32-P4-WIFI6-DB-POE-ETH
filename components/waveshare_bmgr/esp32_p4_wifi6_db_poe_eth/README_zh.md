# ESP32-P4-WIFI6-DB-POE-ETH 板卡定义

[English](README.md)

本目录提供 Waveshare `ESP32-P4-WIFI6-DB-POE-ETH` 的 ESP Board Manager 板卡配置。
板卡芯片为 `esp32p4`，配置版本为 `1.0.0`。它覆盖的 LCD、触摸、音频、摄像头、
SD 卡和 RMII Ethernet 硬件与本板的
[`waveshare_bsp` 版本](../../waveshare_bsp/esp32_p4_wifi6_db_poe_eth/README_zh.md)
一致。

## 默认配置

- LCD：JD9365 MIPI-DSI，800x1280，RGB565，2-lane，1500 Mbps；
- 触摸：GT911，触摸范围 800x1280，I2C 地址候选为 `0xBA` 和 `0x28`；
- 音频：ES8311，I2C 地址 `0x30`，I2S port 0，采样率 48 kHz；
- 摄像头：CSI 摄像头资源，默认配置由 `sdkconfig.defaults.board` 中的 OV5647
  选项提供；
- 存储：SDMMC 4-bit，挂载点 `/sdcard`，默认频率为 `SDMMC_FREQ_HIGHSPEED`；
- MIPI 电源：LDO channel 3，输出 2500 mV；
- LCD 默认使用 3 个 DPI frame buffer，amend 默认关闭；
- Ethernet：RMII MAC + IP101 PHY（`espressif/ip101` 驱动），PHY 地址 `1`，
  默认开启 `CONFIG_ETH_USE_ESP32_EMAC`。

## 外设映射

| 外设 | 配置 |
| --- | --- |
| I2C | port 1，SDA GPIO7，SCL GPIO8 |
| I2S | port 0，MCLK/BCLK/WS/DOUT/DIN 为 GPIO13/12/10/9/11 |
| 功放控制 | GPIO53 |
| SDMMC | CLK/CMD/D0-D3 为 GPIO43/44/39/40/41/42 |
| SD 卡电源 | GPIO45 |
| MIPI-DSI | bus 0，2 lanes，1500 Mbps |
| Ethernet MDC/MDIO | GPIO31 / GPIO52 |
| Ethernet PHY 复位 | GPIO51 |
| RMII 50 MHz 参考时钟输入 | GPIO50（`EMAC_CLK_EXT_IN`，由 PHY 提供） |
| RMII TX_EN/TXD0/TXD1 | GPIO49 / GPIO34 / GPIO35 |
| RMII CRS_DV/RXD0/RXD1 | GPIO28 / GPIO29 / GPIO30 |

Board Manager 未建模（无对应的设备/外设类型）：ESP32-C5 Hosted Wi-Fi 的
SDIO 布线（CLK GPIO18、CMD GPIO19、D0-D3 GPIO14-17、复位 GPIO54、slot 1）。
详见 `board_peripherals.yaml` 末尾的注释说明；该接口由应用层直接配置。

## 文件说明

- `board_info.yaml`：板卡基本信息；
- `board_devices.yaml`：音频、LCD、触摸、摄像头、SD 卡和 Ethernet 设备；
- `board_peripherals.yaml`：总线、GPIO、LDO 和 MIPI-DSI 配置；
- `setup_device.c`：LCD 和 GT911 touch factory hook；
- `ethernet.c`：RMII Ethernet MAC/PHY 的 `custom` 设备 init/deinit 实现；
- `sdkconfig.defaults.board`：默认 Flash、PSRAM、LCD、OV5647 和 Ethernet 配置。

## Ethernet 使用说明

`esp-board-manager` 目前没有内置的 `ethernet` 设备类型，因此本板卡使用
`type: custom` 声明名为 `ethernet` 的设备，遵循
[esp-board-manager 文档](https://github.com/espressif/esp-board-manager)
中描述的 custom 设备模式。`esp_board_manager_init()` 完成后，
获取驱动 handle 并在应用层完成后续初始化（事件循环、netif/glue、
`esp_eth_start()`）：

```c
#include "esp_eth.h"
#include "esp_board_manager.h"

esp_eth_handle_t eth_handle = NULL;
ESP_ERROR_CHECK(esp_board_manager_get_device_handle("ethernet", (void **)&eth_handle));
/* 挂载 netif/glue 后： */
ESP_ERROR_CHECK(esp_eth_start(eth_handle));
```

Ethernet MAC/PHY 初始化需要 `CONFIG_ETH_USE_ESP32_EMAC=y`（已在
`sdkconfig.defaults.board` 中默认开启）；未开启时设备初始化不会返回
有效 handle。

## 使用

在 ESP-IDF 工程目录下执行，`-c` 指向本仓库的 `components/waveshare_bmgr`
目录（板卡包没有发布到组件注册表，不会被自动发现）：

```bash
idf.py bmgr -b esp32_p4_wifi6_db_poe_eth -c <repo>/components/waveshare_bmgr
```

如需使用其他 Waveshare DSI 触摸屏，加上 `-a` 指向 `amends/` 下对应的 LCD amend。
可用的屏幕配置见 [`../amends/README_zh.md`](../amends/README_zh.md)，完整工程见
[`examples/esp-idf/13_board_manager`](../../../examples/esp-idf/13_board_manager/README_zh.md)。

LCD、触摸、摄像头和 Ethernet 参数应与实际硬件版本保持一致；本 README 仅
描述当前仓库中的 Board Manager 配置，不替代硬件验证。
