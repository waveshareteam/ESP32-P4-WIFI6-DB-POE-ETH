set(PROJ_SRC ${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE PROJ_SRCS_C ${PROJ_SRC}/*.c)
list(FILTER PROJ_SRCS_C EXCLUDE REGEX "/assets/img_lv_demo_music_cover_[123](_large)?\\.c$")
file(GLOB_RECURSE PROJ_SRCS_CPP ${PROJ_SRC}/*.cpp)

idf_component_register(
    SRCS  ${PROJ_SRCS_C} ${PROJ_SRCS_CPP}
    INCLUDE_DIRS ${PROJ_SRC} ${PROJ_SRC}/src
    REQUIRES brookesia_core lvgl
    PRIV_REQUIRES spiffs brookesia_service_audio
                  brookesia_service_helper brookesia_service_storage
)

set_source_files_properties(
    ${PROJ_SRCS_CPP}
    PROPERTIES
        COMPILE_FLAGS "-Wno-missing-field-initializers"
)
