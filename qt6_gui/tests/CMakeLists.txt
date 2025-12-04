# Qt6 GUI tests CMakeLists.txt

# Test executable
add_executable(qt6_gui_tests
    test_main_window.cpp
)

target_link_libraries(qt6_gui_tests
    PRIVATE
        qt6_gui_lib
        Qt6::Test
)

# Ensure qt6_gui_lib is built before running tests
add_dependencies(qt6_gui_tests qt6_gui_lib)

# Add tests to CTest with offscreen platform for headless environments
add_test(NAME qt6_gui_tests COMMAND qt6_gui_tests)
set_tests_properties(qt6_gui_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
