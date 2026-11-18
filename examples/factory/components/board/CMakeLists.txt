idf_component_get_property(board_pack_dir waveshare__waveshare_boards COMPONENT_DIR)

set(SRCS "")
list(APPEND SRCS
    "src/board_camera.c"
    "src/board_codec.c"
    "src/board_audio.c"
    "src/board_display.c"
    "src/board_ethernet.c"
    "src/board_storage.c"
    "src/board_wifi.c"
)

set(INCLUDE_DIRS "")
list(APPEND INCLUDE_DIRS "include")

foreach(SRC_DIR IN LISTS SRC_DIRS)
    file(GLOB_RECURSE SRC ${SRC_DIR}/*.c)
    list(APPEND SRCS ${SRC})
    list(APPEND INCLUDE_DIRS ${SRC_DIR}/include)
endforeach()

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    PRIV_INCLUDE_DIRS "${board_pack_dir}/boards/esp32_p4_wifi6_db_poe_eth"
    REQUIRES driver esp_lcd_touch esp_lvgl_adapter esp_board_manager esp_netif
    PRIV_REQUIRES esp_timer fatfs esp_psram esp_mm esp_lcd_touch_gt911 waveshare__waveshare_boards esp_eth esp_event
)

target_compile_options(
    ${COMPONENT_LIB} PRIVATE
        -Wno-format
        -Wno-int-conversion
        -Wno-incompatible-pointer-types
        -Wunused-function
        -Wno-unused-variable
        -Wno-unused-function
        -Wno-overflow
        -Wno-unused-but-set-variable
        -Wno-discarded-qualifiers)
