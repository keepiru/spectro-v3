# Qt6 GUI Tests CMakeLists.txt

find_package(Qt6 REQUIRED COMPONENTS Test)

# test_audio_buffer
add_executable(test_audio_buffer
    test_audio_buffer.cpp
    ../models/audio_buffer.cpp
)

target_link_libraries(test_audio_buffer
    PRIVATE
        spectro_dsp
        Qt6::Core
        Qt6::Test
)

target_include_directories(test_audio_buffer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(NAME test_audio_buffer COMMAND test_audio_buffer)

# test_spectrogram_controller
add_executable(test_spectrogram_controller
    test_spectrogram_controller.cpp
    ../controllers/spectrogram_controller.cpp
    ../models/audio_buffer.cpp
)

target_link_libraries(test_spectrogram_controller
    PRIVATE
        spectro_dsp
        Qt6::Core
        Qt6::Test
)

target_include_directories(test_spectrogram_controller
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(NAME test_spectrogram_controller COMMAND test_spectrogram_controller)
