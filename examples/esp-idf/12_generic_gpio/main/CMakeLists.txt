idf_component_get_property(board_pack_dir waveshare__waveshare_boards COMPONENT_DIR)

idf_component_register(SRCS main.c
    INCLUDE_DIRS "${board_pack_dir}/boards/esp32_p4_wifi6_db_poe_eth"
    REQUIRES gpio_monitor waveshare__waveshare_boards esp_lvgl_adapter esp_lcd_touch lvgl)
