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
        Ethernet
        esp_event esp_netif esp_wifi nvs_flash esp_lvgl_adapter
    REQUIRES ${BMGR_GEN_REQUIRES})

idf_component_get_property(
    MCP_C_SDK_COMPONENT_LIB
    espressif__mcp-c-sdk
    COMPONENT_LIB
)
if(MCP_C_SDK_COMPONENT_LIB)
    target_compile_options(
        ${MCP_C_SDK_COMPONENT_LIB}
        PRIVATE
        -Wno-error=stringop-truncation
    )
endif()

idf_component_get_property(XIAOZHI_FONTS_DIR 78__xiaozhi-fonts COMPONENT_DIR)
if(XIAOZHI_FONTS_DIR AND EXISTS "${XIAOZHI_FONTS_DIR}/cbin/font_puhui_common_20_4.bin")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../spiffs/xiaozhi")
    configure_file(
        "${XIAOZHI_FONTS_DIR}/cbin/font_puhui_common_20_4.bin"
        "${CMAKE_CURRENT_LIST_DIR}/../spiffs/xiaozhi/font.bin"
        COPYONLY
    )
else()
    message(STATUS "xiaozhi-fonts component or common font not found; skipping SPIFFS font copy")
endif()

spiffs_create_partition_image(storage ${CMAKE_CURRENT_LIST_DIR}/../spiffs FLASH_IN_PROJECT)
