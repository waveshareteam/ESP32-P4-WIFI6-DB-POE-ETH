set(audio_sources)
set(display_sources
    src/display/device.cpp
)

set(BMGR_GEN_REQUIRES "")
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../gen_bmgr_codes/CMakeLists.txt")
    set(BMGR_GEN_REQUIRES gen_bmgr_codes)
endif()

if(CONFIG_ESP_HAL_BOARDS_AUDIO_ENABLE)
    list(APPEND audio_sources src/audio/device.cpp src/audio/codec_interfaces.cpp)

    if(CONFIG_ESP_HAL_BOARDS_AUDIO_ENABLE_PROCESSOR)
        list(APPEND audio_sources
            src/audio/processor_impl.cpp
            src/audio/processor_type_converter.cpp
        )
    endif()
endif()

idf_component_register(
    SRCS ${audio_sources} ${display_sources}
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src/audio"
    REQUIRES brookesia_hal_interface brookesia_hal_adaptor esp_lvgl_adapter
        brookesia_service_helper brookesia_service_manager
    PRIV_REQUIRES esp_board_manager ${BMGR_GEN_REQUIRES}
)

if(CONFIG_ESP_HAL_BOARDS_AUDIO_ENABLE)
    idf_component_optional_requires(PRIVATE
        esp_codec_dev
        esp_driver_gpio
    )
    if(CONFIG_ESP_HAL_BOARDS_AUDIO_ENABLE_PROCESSOR)
        idf_component_optional_requires(PRIVATE
            av_processor
            esp-sr
            esp-boost
        )
    endif()

    set(ESP_HAL_BOARDS_AUDIO_DEVICE_PLUGIN_SYMBOL esp_hal_boards_audio_device_symbol)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        ESP_HAL_BOARDS_AUDIO_DEVICE_PLUGIN_SYMBOL=${ESP_HAL_BOARDS_AUDIO_DEVICE_PLUGIN_SYMBOL}
    )
    target_link_libraries(${COMPONENT_LIB} INTERFACE
        "-u ${ESP_HAL_BOARDS_AUDIO_DEVICE_PLUGIN_SYMBOL}"
    )
endif()
