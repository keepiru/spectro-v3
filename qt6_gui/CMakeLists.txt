# Qt6 GUI Application
# This directory contains the Qt6-based graphical user interface,
# including the main application executable. Future alternative GUIs
# (e.g., web-based, command-line) would go in separate directories.

# Uncomment when ready to build Qt6 GUI components:

# # GUI library target (Qt6-dependent components)
# add_library(qt6_gui_lib
#     src/audio_capture.cpp
#     src/spectrogram_view.cpp
#     src/spectrum_plot.cpp
#     src/config_panel.cpp
#     # (add GUI sources as we create them)
# )
# 
# target_include_directories(qt6_gui_lib
#     PUBLIC
#         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#         $<INSTALL_INTERFACE:qt6_gui/include>
#     PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}/src
# )
# 
# target_link_libraries(qt6_gui_lib
#     PUBLIC
#         spectro_dsp        # DSP library (no Qt dependencies)
#         Qt6::Widgets
#         Qt6::Multimedia
# )

# # Main executable (Qt6 GUI application)
# add_executable(spectro
#     src/main.cpp
# )
# 
# target_link_libraries(spectro
#     PRIVATE
#         qt6_gui_lib
# )

# # GUI tests (future)
# add_subdirectory(tests)
