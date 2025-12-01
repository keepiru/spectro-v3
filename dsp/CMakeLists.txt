# DSP Library CMakeLists.txt

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFTW3 REQUIRED fftw3f)

add_library(spectro_lib
    src/fft_processor.cpp
    src/fft_window.cpp
    src/audio_buffer.cpp
)

target_include_directories(spectro_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(spectro_lib
    PUBLIC
        ${FFTW3_LIBRARIES}
)

target_include_directories(spectro_lib SYSTEM PUBLIC
    ${FFTW3_INCLUDE_DIRS}
)

# Add tests subdirectory
add_subdirectory(tests)
