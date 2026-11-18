idf_component_get_property(board_pack_dir waveshare__waveshare_boards COMPONENT_DIR)

idf_component_register(SRCS "main.c" "app_video.c"
    PRIV_INCLUDE_DIRS "." "${board_pack_dir}/boards/esp32_p4_wifi6_db_poe_eth"
    REQUIRES waveshare__waveshare_boards
    PRIV_REQUIRES esp_lvgl_adapter esp_lcd_touch esp_video lvgl esp_driver_ppa esp_mm esp_timer)
