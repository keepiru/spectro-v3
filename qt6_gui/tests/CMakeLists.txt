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

target_link_libraries(test_spectrogram_controller
    PRIVATE
        spectro_qt6_gui
        Qt6::Test
)

add_test(NAME test_spectrogram_controller COMMAND test_spectrogram_controller)
