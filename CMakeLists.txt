# Copyright (c) 2026 ttldtor.
# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 3.20)

project(process)
include(GNUInstallDirs)

set(TTLDTOR_PROCESS "v3.0.2" CACHE STRING "The Process package version")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)

# Determine if process is built as a subproject (using add_subdirectory)
# or if it is the master project.
if (NOT DEFINED TTLDTOR_PROCESS_ROOT_PROJECT)
    set(TTLDTOR_PROCESS_ROOT_PROJECT OFF)
    if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(TTLDTOR_PROCESS_ROOT_PROJECT ON)
        message(STATUS "CMake version: ${CMAKE_VERSION}")
    endif ()
endif ()

option(TTLDTOR_PROCESS_BUILD_SAMPLES "Enable building samples for Process" ${TTLDTOR_PROCESS_ROOT_PROJECT})
option(TTLDTOR_PROCESS_BUILD_TESTS "Enable building tests for Process" ${TTLDTOR_PROCESS_ROOT_PROJECT})

add_library(${PROJECT_NAME} src/process.cpp)
target_compile_features(process PUBLIC cxx_std_14)
set_target_properties(process PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(${PROJECT_NAME} PUBLIC include)

add_library(process::process ALIAS ${PROJECT_NAME})

if (WIN32)
    target_link_libraries(process PRIVATE psapi)
else ()
    message(STATUS "Platform: " ${CMAKE_SYSTEM_NAME})
endif ()

if (TTLDTOR_PROCESS_BUILD_SAMPLES)
    add_executable(${PROJECT_NAME}_sample samples/main.cpp)
    target_link_libraries(${PROJECT_NAME}_sample process)
endif ()

if (TTLDTOR_PROCESS_BUILD_TESTS)
    include(FetchContent)
    include(CTest)

    FetchContent_Declare(doctest
            URL "https://github.com/doctest/doctest/archive/refs/tags/v2.5.3.tar.gz"
            URL_HASH SHA256=174ebc4e769928959614789c5b4e9c3d0a0f81a62bb608756b127bfebfb21331
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(doctest)

    add_executable(${PROJECT_NAME}_test tests/main.cpp)
    target_link_libraries(${PROJECT_NAME}_test process doctest::doctest_with_main)
    target_include_directories(${PROJECT_NAME}_test PRIVATE src)

    enable_testing()
    add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)
endif ()
