# Components

[English](README.md)

本目录提供两种并行的 `esp32_p4_wifi6_db_poe_eth` 板级适配方式。二者针对的是同一块硬件，请根据应用的组织方式选择其一。

| 路径 | 用途 |
| --- | --- |
| [`esp32_p4_wifi6_db_poe_eth/`](esp32_p4_wifi6_db_poe_eth/README_zh.md) | 传统的 ESP-BSP 组件，提供 `bsp_*` C API，涵盖显示、触摸、音频、SD 卡、以太网、USB 和扩展排针。在工程的 `idf_component.yml` 中通过 `path:` / `override_path:` 引入（`examples/esp-idf/` 下的示例即如此），或加入 `EXTRA_COMPONENT_DIRS`，然后直接调用其函数。 |
| [`waveshare_boards/esp32_p4_wifi6_db_poe_eth/`](waveshare_boards/esp32_p4_wifi6_db_poe_eth/) | [ESP Board Manager](https://github.com/espressif/esp-board-manager) 板卡配置（`board_info.yaml` / `board_devices.yaml` / `board_peripherals.yaml`），通过 `idf.py bmgr -b esp32_p4_wifi6_db_poe_eth` 选定，并通过 `esp_board_manager_init()` / `esp_board_manager_get_device_handle()` 使用。LCD 屏幕的 amend 配置位于 [`waveshare_boards/esp32_p4_sub_dev/`](waveshare_boards/esp32_p4_sub_dev/)。 |

引脚映射、支持的外设和用法示例请参阅 BSP 的 README。
