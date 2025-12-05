# Qt6 GUI Application CMakeLists.txt

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia)

# Enable Qt MOC (Meta-Object Compiler) for this directory
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Collect source files
set(GUI_SOURCES
    src/main.cpp
    models/audio_buffer.cpp
)

set(GUI_HEADERS
    models/audio_buffer.h
)

# Create executable
add_executable(spectro
    ${GUI_SOURCES}
    ${GUI_HEADERS}
)

# Link libraries
target_link_libraries(spectro
    PRIVATE
        spectro_dsp
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
)

# Include directories
target_include_directories(spectro
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/models
        ${CMAKE_CURRENT_SOURCE_DIR}/controllers
        ${CMAKE_CURRENT_SOURCE_DIR}/views
)

# Set C++ standard (inherited from root but explicit for clarity)
set_target_properties(spectro PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)
