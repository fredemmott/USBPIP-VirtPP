cmake_minimum_required(VERSION 4.0)

# Suppress bogus 'Manually-specified variables were not used by the project' warning
if (CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Using toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
endif ()

project(usbip_virtpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRE ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(
        usbip_virtpp
        include/FredEmmott/USBIP.hpp
        include/FredEmmott/USBSpec.hpp
        include/FredEmmott/USBIP-VirtPP.h
        src/api-c.cpp
        src/api-detail.hpp
        src/send-recv.cpp src/send-recv.hpp
        src/test.cpp
)
target_include_directories(usbip_virtpp PRIVATE include/)

find_package(wil CONFIG REQUIRED)
target_link_libraries(usbip_virtpp PRIVATE WIL::WIL)