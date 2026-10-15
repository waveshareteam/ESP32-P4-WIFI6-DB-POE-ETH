set(PROJ_SRC ${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE PROJ_SRCS_C ${PROJ_SRC}/*.c)
file(GLOB_RECURSE PROJ_SRCS_CPP ${PROJ_SRC}/*.cpp)

set(BMGR_GEN_REQ "")
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../gen_bmgr_codes/CMakeLists.txt")
    set(BMGR_GEN_REQ gen_bmgr_codes)
endif()

idf_component_register(
    SRCS  ${PROJ_SRCS_C} ${PROJ_SRCS_CPP}
    INCLUDE_DIRS ${PROJ_SRC}
    REQUIRES brookesia_core driver esp_event nvs_flash lwip
        brookesia_service_helper 
        brookesia_service_manager 
        brookesia_service_wifi 
        brookesia_hal_adaptor
        esp_hal_boards
    PRIV_REQUIRES esp_board_manager esp_netif brookesia_service_audio ${BMGR_GEN_REQ}
)

set_source_files_properties(
    ${PROJ_SRCS_CPP}
    PROPERTIES
        COMPILE_FLAGS "-Wno-missing-field-initializers"
)
