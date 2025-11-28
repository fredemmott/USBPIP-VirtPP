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
        src/api/c/detail.hpp
        src/api/c/Device.cpp
        src/api/c/Instance.cpp
        src/api/c/Request.cpp
        src/api/c/send-recv.cpp
        src/api/c/send-recv.hpp
        src/test.cpp
)
target_include_directories(usbip_virtpp PRIVATE include/)
target_compile_options(usbip_virtpp PRIVATE "/EHsc")
target_compile_definitions(usbip_virtpp
        PRIVATE
        NOMINMAX
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
)

find_package(wil CONFIG REQUIRED)
target_link_libraries(usbip_virtpp PRIVATE WIL::WIL)