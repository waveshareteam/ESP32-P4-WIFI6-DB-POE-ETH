# Components

[English](README.md)

本目录提供两种并行的 `esp32_p4_wifi6_db_poe_eth` 板级适配方式，分别位于各自的子目录中。二者针对的是同一块硬件，请根据应用的组织方式选择其一。

| 路径 | 用途 |
| --- | --- |
| [`waveshare_bsp/esp32_p4_wifi6_db_poe_eth/`](waveshare_bsp/esp32_p4_wifi6_db_poe_eth/README_zh.md) | 传统的 ESP-BSP 组件，提供 `bsp_*` C API，涵盖显示、触摸、音频、SD 卡、以太网、USB 和扩展排针。在工程的 `idf_component.yml` 中通过 `path:` / `override_path:` 引入（`examples/esp-idf/` 下的示例即如此），或加入 `EXTRA_COMPONENT_DIRS`，然后直接调用其函数。 |
| [`waveshare_bmgr/esp32_p4_wifi6_db_poe_eth/`](waveshare_bmgr/esp32_p4_wifi6_db_poe_eth/README_zh.md) | [ESP Board Manager](https://github.com/espressif/esp-board-manager) 板卡配置（`board_info.yaml` / `board_devices.yaml` / `board_peripherals.yaml`），通过 `idf.py bmgr -b esp32_p4_wifi6_db_poe_eth` 选定，并通过 `esp_board_manager_init()` / `esp_board_manager_get_device_handle()` 使用。 |

`esp32_p4_wifi6_db_poe_eth` 组件此前是 `components/` 顶层的单一 BSP 组件，现已拆分为上述 `waveshare_bsp/` 与 `waveshare_bmgr/` 两个版本。各子目录的引脚映射、支持的外设和用法示例请参阅其各自的 README。
