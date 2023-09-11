# Copyright (c) 2023 ttldtor.
# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 3.20)

project(process)
include(GNUInstallDirs)

set(TTLDTOR_PROCESS "1.2.0" CACHE STRING "The Process package version")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)
set(CXX_EXTENSIONS OFF)
set(C_EXTENSIONS OFF)

# Determine if process is built as a subproject (using add_subdirectory)
# or if it is the master project.
if (NOT DEFINED TTLDTOR_PROCESS_ROOT_PROJECT)
    set(TTLDTOR_PROCESS_ROOT_PROJECT OFF)
    if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(TTLDTOR_PROCESS_ROOT_PROJECT ON)
        message(STATUS "CMake version: ${CMAKE_VERSION}")
    endif ()
endif ()

option(TTLDTOR_PROCESS_SAMPLES "Enable building samples for Process" ${TTLDTOR_PROCESS_ROOT_PROJECT})

add_library(process src/process.cpp)
target_include_directories(process PUBLIC include)

add_library(process::process ALIAS process)

if (WIN32)
    target_link_libraries(process PRIVATE psapi)
endif ()

if (TTLDTOR_PROCESS_SAMPLES)
    add_executable(process_test test/main.cpp)
    target_link_libraries(process_test process)
endif ()