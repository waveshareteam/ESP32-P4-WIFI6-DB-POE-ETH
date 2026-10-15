set(BMGR_GEN_REQUIRES "")
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../components/gen_bmgr_codes/CMakeLists.txt")
    set(BMGR_GEN_REQUIRES gen_bmgr_codes)
endif()

idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        brookesia_core
        brookesia_app_calculator
        brookesia_app_squareline_demo
        draw
        Camera
        XiaozhiApp
        VideoPlayer
        MusicPlayer
        SpecAnalyzer
        Settings
        esp_event esp_netif esp_wifi nvs_flash esp_lvgl_adapter
    REQUIRES ${BMGR_GEN_REQUIRES} esp_hal_boards)

spiffs_create_partition_image(storage ${CMAKE_CURRENT_LIST_DIR}/../spiffs FLASH_IN_PROJECT)
