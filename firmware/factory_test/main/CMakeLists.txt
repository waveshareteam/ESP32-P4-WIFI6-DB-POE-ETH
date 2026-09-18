set(BMGR_GEN_REQUIRES "")
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../components/gen_bmgr_codes/CMakeLists.txt")
    set(BMGR_GEN_REQUIRES gen_bmgr_codes)
endif()

idf_component_register(
    SRCS
        "main.c"
        "jobs.c"
        "report.c"
        "cmd_sys.c"
        "cmd_test.c"
        "cmd_i2c.c"
        "cmd_storage.c"
        "cmd_usb.c"
        "cmd_net.c"
        "cmd_wifi.c"
        "cmd_display.c"
        "camera_convert.c"
        "camera_preview.c"
        "cmd_camera.c"
        "cmd_audio.c"
        "cmd_gpio.c"
        "cmd_file.c"
    INCLUDE_DIRS "."
        "../../../components/waveshare_bmgr/esp32_p4_wifi6_db_poe_eth"
    REQUIRES esp_board_manager ${BMGR_GEN_REQUIRES} esp_gpio_vfs
    PRIV_REQUIRES console nvs_flash esp_timer esp_psram esp_driver_gpio esp_driver_i2c
        esp_eth esp_netif esp_event esp_wifi fatfs vfs sdmmc usb esp_lcd spi_flash esp_app_format
)

target_link_libraries(${COMPONENT_LIB} PRIVATE m)
