idf_component_register(
    SRCS
        "src/bsp_board_extra.c"
        "src/bsp_board_wifi.c"
    INCLUDE_DIRS "include"
    REQUIRES
        adf_event_hub
        driver
        esp_codec_dev
        esp_driver_gpio
        esp_driver_i2s
        esp_service
        esp_wifi_service
    PRIV_REQUIRES
        esp_config_manager
        esp_driver_ledc
        esp_driver_uart
        esp_mm
        esp_netif
        esp_psram
        esp_timer
        esp_wifi
        esp_wifi_remote
        fatfs
        nvs_flash
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-discarded-qualifiers
    -Wno-format
    -Wno-incompatible-pointer-types
    -Wno-int-conversion
    -Wno-overflow
    -Wno-unused-but-set-variable
    -Wno-unused-function
    -Wno-unused-variable
)
