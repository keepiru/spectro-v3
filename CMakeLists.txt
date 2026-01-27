# Spectro-v3 -- Real-time spectrum analyzer
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2025-2026 Chris "Kai" Frederick

cmake_minimum_required(VERSION 3.25)
project(spectro VERSION 0.1.0 LANGUAGES CXX)

# C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use ccache if available to speed up rebuilds
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "ccache found: ${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# Use mold linker if available for faster linking
include(CheckLinkerFlag)
check_linker_flag(CXX "-fuse-ld=mold" LINKER_SUPPORTS_MOLD)
if(LINKER_SUPPORTS_MOLD)
    message(STATUS "Using mold linker")
    add_link_options("-fuse-ld=mold")
else()
    message(STATUS "Using default linker (mold not found)")
endif()

# Generate compile_commands.json for clang-tidy and other tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable testing at root level
enable_testing()

# Add DSP library subdirectory
add_subdirectory(dsp)

# Add Qt6 GUI subdirectory
add_subdirectory(qt6_gui)
