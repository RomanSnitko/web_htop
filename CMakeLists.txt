cmake_minimum_required(VERSION 3.20)

project(
    web_htop
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Helps clangd/IDE resolve includes after configure.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(WEB_HTOP_BUILD_APPS "Build server/client executables" ON)
option(WEB_HTOP_BUILD_TESTS "Build tests" OFF)

include(cmake/Dependencies.cmake)

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(tests)
