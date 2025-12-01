cmake_minimum_required(VERSION 3.25)
project(spectro VERSION 0.1.0 LANGUAGES CXX)

# C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find dependencies
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFTW3 REQUIRED fftw3f)

# Enable automatic Qt MOC, UIC, RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# DSP Library target (pure C++, no Qt dependencies)
add_library(spectro_lib
    dsp/src/fft_processor.cpp
    dsp/src/fft_window.cpp
    dsp/src/audio_buffer.cpp
)

target_include_directories(spectro_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/dsp/include>
        $<INSTALL_INTERFACE:dsp/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/dsp/src
)

target_link_libraries(spectro_lib
    PUBLIC
        ${FFTW3_LIBRARIES}
)

target_include_directories(spectro_lib SYSTEM PUBLIC
    ${FFTW3_INCLUDE_DIRS}
)

# Qt6 GUI (future - will contain main executable and GUI components)
# add_subdirectory(qt6_gui)

# Enable testing
enable_testing()
add_subdirectory(dsp/tests)
