find_package(Catch2 3 REQUIRED)

add_executable(spectro_tests
    test_main.cpp
)

target_link_libraries(spectro_tests
    PRIVATE
        spectro_lib
        Catch2::Catch2WithMain
)

# Discover tests for CTest
include(CTest)
include(Catch)
catch_discover_tests(spectro_tests)
