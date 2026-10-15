set(PROJ_SRC ${CMAKE_CURRENT_LIST_DIR})

set(PROJ_SRCS_C
    "assets/img_app_xiaozhi.c"
)

set(PROJ_SRCS_CPP
    "XiaozhiActivationClient.cpp"
    "XiaozhiActivationPrompt.cpp"
    "XiaozhiApp.cpp"
    "XiaozhiAudio.cpp"
    "XiaozhiClient.cpp"
    "XiaozhiDeveloperTools.cpp"
    "XiaozhiLocalization.cpp"
    "XiaozhiUi.cpp"
)

set(PROJ_SRCS_CC
    "third_party/xiaozhi_esp32/ogg_demuxer.cc"
)

idf_component_register(
    SRCS ${PROJ_SRCS_C} ${PROJ_SRCS_CPP} ${PROJ_SRCS_CC}
    INCLUDE_DIRS ${PROJ_SRC}
    REQUIRES
        brookesia_core
        xiaozhi-fonts
        esp_event
        esp_netif
        esp_xiaozhi
        nvs_flash
    PRIV_REQUIRES
        brookesia_service_audio
        esp_hal_boards
        app_update
        esp_lvgl_adapter
        spiffs
        esp_http_client
        cjson
        efuse
        esp_hw_support
        esp_app_format
        esp_partition
        esp_timer
        mbedtls
        mcp-c-sdk
        spi_flash
)

# Keep warnings visible while allowing the managed MCP SDK's known
# stringop-truncation warning to compile under this project's -Werror=all.
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

set_source_files_properties(
    ${PROJ_SRCS_CPP} ${PROJ_SRCS_CC}
    PROPERTIES
        COMPILE_FLAGS "-Wno-missing-field-initializers"
)
