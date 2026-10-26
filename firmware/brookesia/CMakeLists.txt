# See https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# Only build the components that main depends on (directly or transitively).
idf_build_set_property(MINIMAL_BUILD ON)
project(esp-brookesia)
