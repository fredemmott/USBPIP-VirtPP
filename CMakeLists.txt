cmake_minimum_required(VERSION 4.0)
project(usbip_virtpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRE ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(
  usbip_virtpp
  test.cpp
  include/FredEmmott/USBIP.hpp
  include/FredEmmott/USBSpec.hpp
)
target_include_directories(usbip_virtpp PRIVATE include/)