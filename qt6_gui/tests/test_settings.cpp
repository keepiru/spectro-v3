#include "include/global_constants.h"
#include "models/settings.h"
#include <QSignalSpy>
#include <catch2/catch_test_macros.hpp>
#include <fft_window.h>

TEST_CASE("Settings::SetFFTSettings emits signal", "[settings]")
{
    Settings settings;
    settings.SetFFTSettings(2048, FFTWindow::Type::Hann);
    const QSignalSpy spy(&settings, &Settings::FFTSettingsChanged);

    settings.SetFFTSettings(4096, FFTWindow::Type::Rectangular);

    REQUIRE(spy.count() == 1);
    REQUIRE(settings.GetFFTSize() == 4096);
    REQUIRE(settings.GetWindowType() == FFTWindow::Type::Rectangular);
}

TEST_CASE("Settings::SetFFTSettings no signal if same values", "[settings]")
{
    Settings settings;
    const QSignalSpy spy(&settings, &Settings::FFTSettingsChanged);
    const int64_t size = settings.GetFFTSize();
    const FFTWindow::Type type = settings.GetWindowType();

    settings.SetFFTSettings(size, type); // Set to default again

    REQUIRE(spy.count() == 0);
}

TEST_CASE("Settings::SetFFTSettings throws on non-positive size", "[settings]")
{
    Settings settings;
    REQUIRE_THROWS_AS(settings.SetFFTSettings(0, settings.GetWindowType()), std::invalid_argument);
    REQUIRE_THROWS_AS(settings.SetFFTSettings(-1024, settings.GetWindowType()),
                      std::invalid_argument);
}

TEST_CASE("Settings::SetFFTSettings throws on non-power of two size", "[settings]")
{
    Settings settings;
    REQUIRE_THROWS_AS(settings.SetFFTSettings(255, settings.GetWindowType()),
                      std::invalid_argument);
}

TEST_CASE("Settings::SetWindowStride emits signal", "[settings]")
{
    Settings settings;
    const QSignalSpy spy(&settings, &Settings::WindowScaleChanged);

    settings.SetWindowScale(2);

    REQUIRE(spy.count() == 1);
    REQUIRE(settings.GetWindowScale() == 2);
}

TEST_CASE("Settings::SetWindowScale throws on invalid values", "[settings]")
{
    Settings settings;
    REQUIRE_THROWS_AS(settings.SetWindowScale(0), std::invalid_argument);
    REQUIRE_THROWS_AS(settings.SetWindowScale(3), std::invalid_argument);
    REQUIRE_THROWS_AS(settings.SetWindowScale(5), std::invalid_argument);
    REQUIRE_THROWS_AS(settings.SetWindowScale(32), std::invalid_argument);
}

TEST_CASE("Settings::GetStride computes stride", "[settings]")
{
    Settings settings;

    settings.SetFFTSettings(2048, FFTWindow::Type::Hann);
    settings.SetWindowScale(4);
    REQUIRE(settings.GetWindowStride() == 512);

    settings.SetWindowScale(8);
    REQUIRE(settings.GetWindowStride() == 256);

    settings.SetFFTSettings(1024, FFTWindow::Type::Rectangular);
    settings.SetWindowScale(1);
    REQUIRE(settings.GetWindowStride() == 1024);

    settings.SetWindowScale(2);
    REQUIRE(settings.GetWindowStride() == 512);
}

TEST_CASE("Settings::GetApertureMinDecibels", "[settings]")
{
    const Settings settings;
    REQUIRE(settings.GetApertureMinDecibels() == -30.0f);
}

TEST_CASE("Settings::GetApertureMaxDecibels", "[settings]")
{
    const Settings settings;
    REQUIRE(settings.GetApertureMaxDecibels() == 30.0f);
}

TEST_CASE("Settings::SetColorMap invalid", "[settings]")
{
    Settings settings;

    // Invalid enum value (not in defined range)
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    const auto invalidValue = static_cast<Settings::ColorMapType>(999);
    REQUIRE_THROWS_AS(settings.SetColorMap(0, invalidValue), std::invalid_argument);

    // Also verify the Count sentinel is not accepted
    REQUIRE_THROWS_AS(settings.SetColorMap(0, Settings::ColorMapType::Count),
                      std::invalid_argument);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity) -- caused by macro expansion
TEST_CASE("Settings default color maps", "[settings]")
{
    const Settings settings;

    for (size_t i = 0; i < 256; i++) {
        const auto intensity = static_cast<uint8_t>(i);
        // Channel 0: Cyan
        REQUIRE(settings.GetColorMapValue(0, i).r == 0);
        REQUIRE(settings.GetColorMapValue(0, i).g == intensity);
        REQUIRE(settings.GetColorMapValue(0, i).b == intensity);
        // Channel 1: Red
        REQUIRE(settings.GetColorMapValue(1, i).r == intensity);
        REQUIRE(settings.GetColorMapValue(1, i).g == 0);
        REQUIRE(settings.GetColorMapValue(1, i).b == 0);
        // Rest of channels: White
        for (size_t ch = 2; ch < gkMaxChannels; ch++) {
            REQUIRE(settings.GetColorMapValue(ch, i).r == intensity);
            REQUIRE(settings.GetColorMapValue(ch, i).g == intensity);
            REQUIRE(settings.GetColorMapValue(ch, i).b == intensity);
        }
    }
}

TEST_CASE("Settings::GetColorMap", "[settings]")
{
    Settings settings;

    // Confirm default color maps
    REQUIRE(settings.GetColorMap(0) == Settings::ColorMapType::Cyan);
    REQUIRE(settings.GetColorMap(1) == Settings::ColorMapType::Red);
    for (size_t ch = 2; ch < gkMaxChannels; ch++) {
        REQUIRE(settings.GetColorMap(ch) == Settings::ColorMapType::White);
    }

    // Then change and confirm
    settings.SetColorMap(0, Settings::ColorMapType::Blue);
    REQUIRE(settings.GetColorMap(0) == Settings::ColorMapType::Blue);
}

TEST_CASE("Settings::GetColorMapValue out of range", "[settings]")
{
    const Settings settings;

    // Index out of range should throw
    REQUIRE_THROWS_AS((void)settings.GetColorMapValue(gkMaxChannels, 0), std::out_of_range);
}

TEST_CASE("Settings::SetAperture", "[settings]")
{
    Settings settings;
    const QSignalSpy spy(&settings, &Settings::ApertureSettingsChanged);

    settings.SetApertureMinDecibels(-40.0f);
    settings.SetApertureMaxDecibels(20.0f);
    REQUIRE(spy.count() == 2);
    REQUIRE(settings.GetApertureMinDecibels() == -40.0f);
    REQUIRE(settings.GetApertureMaxDecibels() == 20.0f);
}
