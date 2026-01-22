# Spectro-v3 -- Real-time spectrum analyzer
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2025-2026 Chris "Kai" Frederick

# DSP Library CMakeLists.txt

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFTW3 REQUIRED fftw3f)

add_library(spectro_dsp
    src/fft_processor.cpp
    src/fft_window.cpp
    src/sample_buffer.cpp
)

target_include_directories(spectro_dsp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(spectro_dsp
    PUBLIC
        ${FFTW3_LIBRARIES}
)

target_include_directories(spectro_dsp SYSTEM PUBLIC
    ${FFTW3_INCLUDE_DIRS}
)

# Add tests subdirectory
add_subdirectory(tests)
