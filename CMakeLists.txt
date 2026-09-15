cmake_minimum_required(VERSION 3.20)
project(web_htop VERSION 0.2.0 LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "web_htop requires Linux (WSL2 is supported)")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(WebHtopProjectOptions)
web_htop_configure_project_options()

find_package(Threads REQUIRED)
include(CTest)

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client)

if(WEB_HTOP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
