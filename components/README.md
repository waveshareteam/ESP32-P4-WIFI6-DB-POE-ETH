# Components

[中文](README_zh.md)

This directory holds two parallel ways to bring up the
`esp32_p4_wifi6_db_poe_eth` board. Both describe the same physical hardware;
pick the one that matches how your application is structured.

| Path | Purpose |
| --- | --- |
| [`esp32_p4_wifi6_db_poe_eth/`](esp32_p4_wifi6_db_poe_eth/README.md) | Traditional ESP-BSP component exposing `bsp_*` C APIs for display, touch, audio, SD card, Ethernet, USB, and the expansion header. Reference it from the project's `idf_component.yml` (`path:` / `override_path:`, as the examples under `examples/esp-idf/` do) or add it to `EXTRA_COMPONENT_DIRS`, then call its functions directly. |
| [`waveshare_boards/esp32_p4_wifi6_db_poe_eth/`](waveshare_boards/esp32_p4_wifi6_db_poe_eth/) | [ESP Board Manager](https://github.com/espressif/esp-board-manager) board definition (`board_info.yaml` / `board_devices.yaml` / `board_peripherals.yaml`) for the same board, selected with `idf.py bmgr -b esp32_p4_wifi6_db_poe_eth` and consumed through `esp_board_manager_init()` / `esp_board_manager_get_device_handle()`. LCD panel amendments are under [`waveshare_boards/esp32_p4_sub_dev/`](waveshare_boards/esp32_p4_sub_dev/). |

See the BSP README for pin mapping, supported peripherals, and usage examples.
