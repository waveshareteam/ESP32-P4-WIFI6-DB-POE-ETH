# Components

[中文](README_zh.md)

This directory holds two parallel ways to bring up the
`esp32_p4_wifi6_db_poe_eth` board, each in its own subdirectory. Both describe
the same physical hardware; pick the one that matches how your application is
structured.

| Path | Purpose |
| --- | --- |
| [`waveshare_bsp/esp32_p4_wifi6_db_poe_eth/`](waveshare_bsp/esp32_p4_wifi6_db_poe_eth/README.md) | Traditional ESP-BSP component exposing `bsp_*` C APIs for display, touch, audio, SD card, Ethernet, USB, and the expansion header. Reference it from the project's `idf_component.yml` (`path:` / `override_path:`, as the examples under `examples/esp-idf/` do) or add it to `EXTRA_COMPONENT_DIRS`, then call its functions directly. |
| [`waveshare_bmgr/esp32_p4_wifi6_db_poe_eth/`](waveshare_bmgr/esp32_p4_wifi6_db_poe_eth/README.md) | [ESP Board Manager](https://github.com/espressif/esp-board-manager) board definition (`board_info.yaml` / `board_devices.yaml` / `board_peripherals.yaml`) for the same board, selected with `idf.py bmgr -b esp32_p4_wifi6_db_poe_eth` and consumed through `esp_board_manager_init()` / `esp_board_manager_get_device_handle()`. |

Both `esp32_p4_wifi6_db_poe_eth` components were previously a single BSP
component at the top level of `components/`; it has since been split into the
`waveshare_bsp/` and `waveshare_bmgr/` variants above. See each subdirectory's own README for pin
mapping, supported peripherals, and usage examples.
