# Qt6 GUI CMakeLists.txt

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia Test)

# Enable Qt's meta-object compiler (MOC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# GUI library sources
add_library(qt6_gui_lib
    src/main_window.cpp
    include/main_window.h
)

target_include_directories(qt6_gui_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(qt6_gui_lib
    PUBLIC
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
        spectro_dsp
)

# Main executable
add_executable(spectro
    src/main.cpp
)

target_link_libraries(spectro
    PRIVATE
        qt6_gui_lib
)

# Add tests subdirectory
add_subdirectory(tests)
