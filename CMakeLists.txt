cmake_minimum_required(VERSION 3.25)
project(spectro VERSION 0.1.0 LANGUAGES CXX)

# C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable testing at root level
enable_testing()

# Add DSP library subdirectory
add_subdirectory(dsp)

# Qt6 GUI (future - will contain main executable and GUI components)
# add_subdirectory(qt6_gui)
