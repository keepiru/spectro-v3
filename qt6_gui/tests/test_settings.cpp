// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "include/global_constants.h"
#include "models/settings.h"
#include <QSignalSpy>
#include <catch2/catch_test_macros.hpp>
#include <fft_window.h>

TEST_CASE("Settings::SetFFTSettings emits signals", "[settings]")
{
    Settings settings;
    settings.SetFFTSettings(2048, FFTWindow::Type::Hann);
    const QSignalSpy fftSpy(&settings, &Settings::FFTSettingsChanged);
    const QSignalSpy displaySpy(&settings, &Settings::DisplaySettingsChanged);

    settings.SetFFTSettings(4096, FFTWindow::Type::Rectangular);

    REQUIRE(fftSpy.count() == 1);
    REQUIRE(displaySpy.count() == 1);
    REQUIRE(settings.GetFFTSize() == 4096);
    REQUIRE(settings.GetWindowType() == FFTWindow::Type::Rectangular);
}

TEST_CASE("Settings::SetFFTSettings no signal if same values", "[settings]")
{
    Settings settings;
    const QSignalSpy fftSpy(&settings, &Settings::FFTSettingsChanged);
    const QSignalSpy displaySpy(&settings, &Settings::DisplaySettingsChanged);
    const FFTSize size = settings.GetFFTSize();
    const FFTWindow::Type type = settings.GetWindowType();

    settings.SetFFTSettings(size, type); // Set to default again

    REQUIRE(fftSpy.count() == 0);
    REQUIRE(displaySpy.count() == 0);
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

TEST_CASE("Settings::SetWindowScale emits signal", "[settings]")
{
    Settings settings;
    const QSignalSpy spy(&settings, &Settings::DisplaySettingsChanged);

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
        // Channel 0: Magenta
        REQUIRE(settings.GetColorMapValue(0, i).r == intensity);
        REQUIRE(settings.GetColorMapValue(0, i).g == 0);
        REQUIRE(settings.GetColorMapValue(0, i).b == intensity);
        // Channel 1: Green
        REQUIRE(settings.GetColorMapValue(1, i).r == 0);
        REQUIRE(settings.GetColorMapValue(1, i).g == intensity);
        REQUIRE(settings.GetColorMapValue(1, i).b == 0);
        // Rest of channels: White
        for (ChannelCount ch = 2; ch < GKMaxChannels; ch++) {
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
    REQUIRE(settings.GetColorMap(0) == Settings::ColorMapType::Magenta);
    REQUIRE(settings.GetColorMap(1) == Settings::ColorMapType::Green);
    for (ChannelCount ch = 2; ch < GKMaxChannels; ch++) {
        REQUIRE(settings.GetColorMap(ch) == Settings::ColorMapType::White);
    }

    const QSignalSpy spy(&settings, &Settings::DisplaySettingsChanged);
    // Then change and confirm
    settings.SetColorMap(0, Settings::ColorMapType::Blue);
    REQUIRE(spy.count() == 1);
    REQUIRE(settings.GetColorMap(0) == Settings::ColorMapType::Blue);
}

TEST_CASE("Settings::GetColorMapValue out of range", "[settings]")
{
    const Settings settings;

    // Index out of range should throw
    REQUIRE_THROWS_AS((void)settings.GetColorMapValue(GKMaxChannels, 0), std::out_of_range);
}

TEST_CASE("Settings::SetColorMap colormap values", "[settings]")
{
    Settings settings;

    SECTION("Disabled colormap is black")
    {
        settings.SetColorMap(0, Settings::ColorMapType::Disabled);
        for (size_t i = 0; i < 256; i++) {
            const auto entry = settings.GetColorMapValue(0, i);
            REQUIRE(entry.r == 0);
            REQUIRE(entry.g == 0);
            REQUIRE(entry.b == 0);
        }
    }

    SECTION("White colormap is white")
    {
        settings.SetColorMap(0, Settings::ColorMapType::White);
        for (size_t i = 0; i < 256; i++) {
            const auto entry = settings.GetColorMapValue(0, i);
            const auto intensity = static_cast<uint8_t>(i);
            REQUIRE(entry.r == intensity);
            REQUIRE(entry.g == intensity);
            REQUIRE(entry.b == intensity);
        }
    }

    SECTION("Red colormap is red gradient")
    {
        settings.SetColorMap(0, Settings::ColorMapType::Red);
        for (size_t i = 0; i < 256; i++) {
            const auto entry = settings.GetColorMapValue(0, i);
            const auto intensity = static_cast<uint8_t>(i);
            REQUIRE(entry.r == intensity);
            REQUIRE(entry.g == 0);
            REQUIRE(entry.b == 0);
        }
    }

    SECTION("Viridis colormap has expected RGB values")
    {
        settings.SetColorMap(0, Settings::ColorMapType::Viridis);
        // Check key points in the Viridis colormap
        const auto entry0 = settings.GetColorMapValue(0, 0);
        const auto entry128 = settings.GetColorMapValue(0, 128);
        const auto entry255 = settings.GetColorMapValue(0, 255);

        // Viridis starts dark purple, mid teal, ends yellow
        REQUIRE(entry0.r == 68);
        REQUIRE(entry0.g == 1);
        REQUIRE(entry0.b == 84);
        REQUIRE(entry128.r == 33);
        REQUIRE(entry128.g == 145);
        REQUIRE(entry128.b == 140);
        REQUIRE(entry255.r == 253);
        REQUIRE(entry255.g == 231);
        REQUIRE(entry255.b == 37);
    }

    SECTION("Plasma colormap has expected RGB values")
    {
        settings.SetColorMap(0, Settings::ColorMapType::Plasma);
        const auto entry0 = settings.GetColorMapValue(0, 0);
        const auto entry128 = settings.GetColorMapValue(0, 128);
        const auto entry255 = settings.GetColorMapValue(0, 255);

        // Plasma starts dark blue, mid magenta, ends yellow
        REQUIRE(entry0.r == 13);
        REQUIRE(entry0.g == 8);
        REQUIRE(entry0.b == 135);
        REQUIRE(entry128.r == 204);
        REQUIRE(entry128.g == 71);
        REQUIRE(entry128.b == 120);
        REQUIRE(entry255.r == 240);
        REQUIRE(entry255.g == 249);
        REQUIRE(entry255.b == 33);
    }

    SECTION("Inferno colormap has expected RGB values")
    {
        settings.SetColorMap(0, Settings::ColorMapType::Inferno);
        const auto entry0 = settings.GetColorMapValue(0, 0);
        const auto entry128 = settings.GetColorMapValue(0, 128);
        const auto entry255 = settings.GetColorMapValue(0, 255);

        // Inferno starts black, mid red-orange, ends bright yellow
        REQUIRE(entry0.r == 0);
        REQUIRE(entry0.g == 0);
        REQUIRE(entry0.b == 4);
        REQUIRE(entry128.r == 188);
        REQUIRE(entry128.g == 55);
        REQUIRE(entry128.b == 84);
        REQUIRE(entry255.r == 252);
        REQUIRE(entry255.g == 255);
        REQUIRE(entry255.b == 164);
    }

    SECTION("Magma colormap has expected RGB values")
    {
        settings.SetColorMap(0, Settings::ColorMapType::Magma);
        const auto entry0 = settings.GetColorMapValue(0, 0);
        const auto entry128 = settings.GetColorMapValue(0, 128);
        const auto entry255 = settings.GetColorMapValue(0, 255);

        // Magma starts black, mid purple-pink, ends light yellow
        REQUIRE(entry0.r == 0);
        REQUIRE(entry0.g == 0);
        REQUIRE(entry0.b == 4);
        REQUIRE(entry128.r == 183);
        REQUIRE(entry128.g == 55);
        REQUIRE(entry128.b == 121);
        REQUIRE(entry255.r == 252);
        REQUIRE(entry255.g == 253);
        REQUIRE(entry255.b == 191);
    }
}

TEST_CASE("Settings::SetAperture", "[settings]")
{
    Settings settings;
    const QSignalSpy spy(&settings, &Settings::DisplaySettingsChanged);

    settings.SetApertureMinDecibels(-40.0f);
    settings.SetApertureMaxDecibels(20.0f);
    REQUIRE(spy.count() == 2);
    REQUIRE(settings.GetApertureMinDecibels() == -40.0f);
    REQUIRE(settings.GetApertureMaxDecibels() == 20.0f);
}

TEST_CASE("Settings Live Mode", "[settings]")
{
    Settings settings;

    SECTION("default is true")
    {
        REQUIRE(settings.IsLiveMode() == true);
    }

    SECTION("SetLiveMode changes value")
    {
        settings.SetLiveMode(true);
        REQUIRE(settings.IsLiveMode() == true);

        settings.SetLiveMode(false);
        REQUIRE(settings.IsLiveMode() == false);
    }

    SECTION("ClearLiveMode sets to false")
    {
        settings.SetLiveMode(true);
        REQUIRE(settings.IsLiveMode() == true);

        settings.ClearLiveMode();
        REQUIRE(settings.IsLiveMode() == false);
    }
}
