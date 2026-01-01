find_package(Catch2 3 REQUIRED)

add_executable(spectro_dsp_tests
    test_audio_types.cpp
    test_fft_processor.cpp
    test_fft_window.cpp
    test_sample_buffer.cpp
    test_mock_fft_processor.cpp
)

target_link_libraries(spectro_dsp_tests
    PRIVATE
        spectro_dsp
        Catch2::Catch2WithMain
)

# Ensure spectro_dsp is built before running tests
add_dependencies(spectro_dsp_tests spectro_dsp)

# Discover tests for CTest
include(CTest)
include(Catch)
catch_discover_tests(spectro_dsp_tests)
