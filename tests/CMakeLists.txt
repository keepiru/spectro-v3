find_package(Catch2 3 REQUIRED)

add_executable(spectro_tests
    test_main.cpp
    test_fft_processor.cpp
    test_fft_window.cpp
    test_audio_buffer.cpp
)

target_link_libraries(spectro_tests
    PRIVATE
        spectro_lib
        Catch2::Catch2WithMain
)

# Ensure spectro_lib is built before running tests
add_dependencies(spectro_tests spectro_lib)

# Discover tests for CTest
include(CTest)
include(Catch)
catch_discover_tests(spectro_tests)
