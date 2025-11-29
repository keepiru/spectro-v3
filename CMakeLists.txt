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

# Library target
add_library(spectro_lib
    # src/fft_processor.cpp
    # src/audio_capture.cpp
    # (add sources as we create them)
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
        Qt6::Widgets
        Qt6::Multimedia
        ${FFTW3_LIBRARIES}
)

target_include_directories(spectro_lib SYSTEM PUBLIC
    ${FFTW3_INCLUDE_DIRS}
)

# Main executable (placeholder for future)
# add_executable(spectro src/main.cpp)
# target_link_libraries(spectro PRIVATE spectro_lib)

# Enable testing
enable_testing()
add_subdirectory(tests)
