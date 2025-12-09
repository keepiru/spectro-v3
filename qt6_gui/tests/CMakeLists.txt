# Qt6 GUI Tests CMakeLists.txt

find_package(Qt6 REQUIRED COMPONENTS Test)

# test_audio_buffer
add_executable(test_audio_buffer
    test_audio_buffer.cpp
)

target_link_libraries(test_audio_buffer
    PRIVATE
        spectro_qt6_gui
        Qt6::Test
)

add_test(NAME test_audio_buffer COMMAND test_audio_buffer)

# test_spectrogram_controller
add_executable(test_spectrogram_controller
    test_spectrogram_controller.cpp
)

target_include_directories(test_spectrogram_controller
    PRIVATE
        ${CMAKE_SOURCE_DIR}/dsp/tests
)

target_link_libraries(test_spectrogram_controller
    PRIVATE
        spectro_qt6_gui
        Qt6::Test
)

add_test(NAME test_spectrogram_controller COMMAND test_spectrogram_controller)

# test_spectrogram_view
add_executable(test_spectrogram_view
    test_spectrogram_view.cpp
)

target_link_libraries(test_spectrogram_view
    PRIVATE
        spectro_qt6_gui
        Qt6::Test
)

add_test(NAME test_spectrogram_view COMMAND test_spectrogram_view)
set_tests_properties(test_spectrogram_view PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

# test_spectrum_plot
add_executable(test_spectrum_plot
    test_spectrum_plot.cpp
)

target_link_libraries(test_spectrum_plot
    PRIVATE
        spectro_qt6_gui
        Qt6::Test
)

add_test(NAME test_spectrum_plot COMMAND test_spectrum_plot)
set_tests_properties(test_spectrum_plot PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

# test_config_panel
add_executable(test_config_panel
    test_config_panel.cpp
)

target_link_libraries(test_config_panel
    PRIVATE
        spectro_qt6_gui
        Qt6::Test
)

add_test(NAME test_config_panel COMMAND test_config_panel)
set_tests_properties(test_config_panel PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
