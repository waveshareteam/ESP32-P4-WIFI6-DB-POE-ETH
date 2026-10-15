# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# WIN32 describes the target platform. This cross-compiled ESP-IDF target is
# not Windows, even when CMake itself runs on Windows.
if(CMAKE_HOST_WIN32 AND CMAKE_GENERATOR MATCHES "Ninja")
    set(CMAKE_NINJA_FORCE_RESPONSE_FILE ON CACHE BOOL
        "Force Ninja to pass compiler arguments through response files" FORCE)
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(esp-brookesia)
