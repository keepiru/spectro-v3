# Spectro-v3 -- Real-time spectrum analyzer
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2025-2026 Chris "Kai" Frederick

# Qt6 GUI Tests CMakeLists.txt

find_package(Qt6 REQUIRED COMPONENTS Test)
find_package(Catch2 3 REQUIRED)

# Shared test main library - compiled once, linked to all tests in this directory.
# This is an internal helper for qt6_gui tests only and is not intended for use
# by other modules or test suites. It speeds up builds and clang-tidy (heavy
# Qt/Catch2 headers are parsed only once).
add_library(spectro_test_main STATIC test_main_qt.cpp)
target_link_libraries(spectro_test_main
    PUBLIC
        Qt6::Widgets
        Catch2::Catch2
)

# Helper function to create Qt tests with Catch2
function(add_qt_test TEST_NAME)
    cmake_parse_arguments(ARG "" "INCLUDE_DIR" "" ${ARGN})
    
    add_executable(${TEST_NAME} 
        ${TEST_NAME}.cpp
    )
    
    if(ARG_INCLUDE_DIR)
        target_include_directories(${TEST_NAME} PRIVATE ${ARG_INCLUDE_DIR})
    endif()
    
    target_link_libraries(${TEST_NAME}
        PRIVATE
            spectro_qt6_gui
            spectro_test_main
            Qt6::Test
    )
    
    # Option A: batched tests
    
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME} --colour-mode ansi)
    set_tests_properties(${TEST_NAME} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    # Option B: individual tests:
    #catch_discover_tests(${TEST_NAME} TEST_SPEC "--colour-mode ansi"
    #PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

# Discover tests for CTest
include(CTest)


# Define all tests
add_qt_test(test_audio_buffer)
add_qt_test(test_audio_buffer_qiodevice)
add_qt_test(test_audio_file)
set_tests_properties(test_audio_file PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_qt_test(test_audio_recorder)
add_qt_test(test_colormap)
add_qt_test(test_settings)
add_qt_test(test_audio_file_reader)
set_tests_properties(test_audio_file_reader PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_qt_test(test_spectrogram_controller INCLUDE_DIR ${CMAKE_SOURCE_DIR}/dsp/tests)
add_qt_test(test_spectrogram_view INCLUDE_DIR ${CMAKE_SOURCE_DIR}/dsp/tests)
add_qt_test(test_scale_view INCLUDE_DIR ${CMAKE_SOURCE_DIR}/dsp/tests)
add_qt_test(test_spectrum_plot INCLUDE_DIR ${CMAKE_SOURCE_DIR}/dsp/tests)
add_qt_test(test_settings_panel)
add_qt_test(test_settings_controller)
