# Copyright (c) 2023 ttldtor.
# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 3.26)
project(process)

set(CMAKE_CXX_STANDARD 20)

add_library(process src/process.cpp)
target_include_directories(process PUBLIC include)

if (WIN32)
    target_link_libraries(process PRIVATE psapi)
endif ()

add_executable(process_test test/main.cpp)
target_link_libraries(process_test process)