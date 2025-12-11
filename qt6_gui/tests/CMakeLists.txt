# Qt6 GUI Tests CMakeLists.txt

find_package(Qt6 REQUIRED COMPONENTS Test)

# Helper function to create Qt tests
function(add_qt_test TEST_NAME)
    cmake_parse_arguments(ARG "" "INCLUDE_DIR" "" ${ARGN})
    
    add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    
    if(ARG_INCLUDE_DIR)
        target_include_directories(${TEST_NAME} PRIVATE ${ARG_INCLUDE_DIR})
    endif()
    
    target_link_libraries(${TEST_NAME}
        PRIVATE
            spectro_qt6_gui
            Qt6::Test
    )
    
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    
    set_tests_properties(${TEST_NAME} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endfunction()

# Define all tests
add_qt_test(test_audio_buffer)
add_qt_test(test_audio_recorder)
add_qt_test(test_spectrogram_controller INCLUDE_DIR ${CMAKE_SOURCE_DIR}/dsp/tests)
add_qt_test(test_spectrogram_view)
add_qt_test(test_spectrum_plot)
add_qt_test(test_config_panel)
