idf_component_get_property(board_pack_dir waveshare__waveshare_boards COMPONENT_DIR)

idf_component_register(SRCS "test_esp_lcd_jd9365.c"
    INCLUDE_DIRS "${board_pack_dir}/boards/esp32_p4_wifi6_db_poe_eth"
    REQUIRES waveshare__waveshare_boards esp_lcd esp_driver_gpio)
