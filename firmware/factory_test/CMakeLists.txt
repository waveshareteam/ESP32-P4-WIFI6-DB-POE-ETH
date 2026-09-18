cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# components/gen_bmgr_codes is produced by:
#   idf.py bmgr -b esp32_p4_wifi6_db_poe_eth -c ../../components/waveshare_bmgr
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/components/gen_bmgr_codes/CMakeLists.txt")
    message(WARNING
        "components/gen_bmgr_codes not found - the build will fail. Run first:\n"
        "  idf.py bmgr -b esp32_p4_wifi6_db_poe_eth -c ../../components/waveshare_bmgr")
endif()

project(factory_test)
