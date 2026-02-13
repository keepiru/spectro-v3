# Spectro-v3 -- Real-time spectrum analyzer
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2025-2026 Chris "Kai" Frederick

# Qt6 GUI Application CMakeLists.txt

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia)

# Find libsndfile
find_package(PkgConfig REQUIRED)
pkg_check_modules(SNDFILE REQUIRED sndfile)

# Enable Qt MOC (Meta-Object Compiler) for this directory
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Create GUI library (models, views, and controllers)
add_library(spectro_qt6_gui
    adapters/audio_buffer_qiodevice.cpp
    adapters/audio_file_reader.cpp
    adapters/media_devices.cpp
    controllers/audio_file.cpp
    controllers/audio_player.cpp
    controllers/audio_recorder.cpp
    controllers/settings_controller.cpp
    controllers/spectrogram_controller.cpp
    models/audio_buffer.cpp
    models/colormap.cpp
    models/settings.cpp
    views/scale_view.cpp
    views/settings_panel.cpp
    views/spectrogram_view.cpp
    views/spectrum_plot.cpp
)

target_link_libraries(spectro_qt6_gui
    PUBLIC
        spectro_dsp
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
        ${SNDFILE_LIBRARIES}
)

target_include_directories(spectro_qt6_gui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${SNDFILE_INCLUDE_DIRS}
)

set_target_properties(spectro_qt6_gui PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Create executable
add_executable(spectro
    src/main.cpp
    src/main_window.cpp
)

# Link libraries
target_link_libraries(spectro
    PRIVATE
        spectro_qt6_gui
        Qt6::Widgets
)

# Add tests subdirectory
add_subdirectory(tests)
