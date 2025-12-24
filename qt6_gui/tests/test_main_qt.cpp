// Custom Catch2 main with QApplication initialization
// This allows Qt GUI tests to run with Catch2 framework

#define CATCH_CONFIG_RUNNER
#include <catch2/catch_session.hpp>

#include <QApplication>

int
main(int argc, char* argv[])
{
    // Initialize QApplication for Qt GUI testing
    // This is required for widget tests and signal/slot mechanisms
    QApplication const app(argc, argv);

    // Run Catch2 test session
    return Catch::Session().run(argc, argv);
}
