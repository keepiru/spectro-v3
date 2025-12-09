# Qt6 GUI Application CMakeLists.txt

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia)

# Enable Qt MOC (Meta-Object Compiler) for this directory
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Create GUI library (models and controllers)
add_library(spectro_qt6_gui
    models/audio_buffer.cpp
    models/audio_buffer.h
    controllers/spectrogram_controller.cpp
    controllers/spectrogram_controller.h
)

target_link_libraries(spectro_qt6_gui
    PUBLIC
        spectro_dsp
        Qt6::Core
        Qt6::Multimedia
)

target_include_directories(spectro_qt6_gui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/models
        ${CMAKE_CURRENT_SOURCE_DIR}/controllers
)

set_target_properties(spectro_qt6_gui PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Create executable
add_executable(spectro
    src/main.cpp
    src/main_window.h
    src/main_window.cpp
)

# Link libraries
target_link_libraries(spectro
    PRIVATE
        spectro_qt6_gui
        Qt6::Widgets
)

# Set C++ standard (inherited from root but explicit for clarity)
set_target_properties(spectro PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Add tests subdirectory
add_subdirectory(tests)
