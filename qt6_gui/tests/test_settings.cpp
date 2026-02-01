// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "audio_types.h"
#include "include/global_constants.h"
#include "models/colormap.h"
#include "models/settings.h"
#include <QSignalSpy>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <fft_window.h>
#include <stdexcept>

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

TEST_CASE("Settings::GetApertureFloorDecibels", "[settings]")
{
    const Settings settings;
    REQUIRE(settings.GetApertureFloorDecibels() == -20.0f);
}

TEST_CASE("Settings::GetApertureCeilingDecibels", "[settings]")
{
    const Settings settings;
    REQUIRE(settings.GetApertureCeilingDecibels() == 40.0f);
}

TEST_CASE("Settings::SetColorMap invalid", "[settings]")
{
    Settings settings;

    // Invalid enum value (not in defined range)
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    const auto invalidValue = static_cast<ColorMap::Type>(999);
    REQUIRE_THROWS_AS(settings.SetColorMap(0, invalidValue), std::invalid_argument);

    // Also verify the Count sentinel is not accepted
    REQUIRE_THROWS_AS(settings.SetColorMap(0, ColorMap::Type::Count), std::invalid_argument);
}

TEST_CASE("Settings default color maps", "[settings]")
{
    const Settings settings;
    const auto& luts = settings.GetColorMapLUTs();

    for (size_t i = 0; i < 256; i++) {
        const auto intensity = static_cast<uint8_t>(i);
        // Channel 0: Magenta
        REQUIRE(luts.at(0).at(i).r == intensity);
        REQUIRE(luts.at(0).at(i).g == 0);
        REQUIRE(luts.at(0).at(i).b == intensity);
        // Channel 1: Green
        REQUIRE(luts.at(1).at(i).r == 0);
        REQUIRE(luts.at(1).at(i).g == intensity);
        REQUIRE(luts.at(1).at(i).b == 0);
        // Rest of channels: White
        for (ChannelCount ch = 2; ch < GKMaxChannels; ch++) {
            REQUIRE(luts.at(ch).at(i).r == intensity);
            REQUIRE(luts.at(ch).at(i).g == intensity);
            REQUIRE(luts.at(ch).at(i).b == intensity);
        }
    }
}

TEST_CASE("Settings::GetColorMap", "[settings]")
{
    Settings settings;

    // Confirm default color maps
    REQUIRE(settings.GetColorMap(0) == ColorMap::Type::Magenta);
    REQUIRE(settings.GetColorMap(1) == ColorMap::Type::Green);
    for (ChannelCount ch = 2; ch < GKMaxChannels; ch++) {
        REQUIRE(settings.GetColorMap(ch) == ColorMap::Type::White);
    }

    const QSignalSpy spy(&settings, &Settings::DisplaySettingsChanged);
    // Then change and confirm
    settings.SetColorMap(0, ColorMap::Type::Blue);
    REQUIRE(spy.count() == 1);
    REQUIRE(settings.GetColorMap(0) == ColorMap::Type::Blue);
}

TEST_CASE("Settings::SetAperture", "[settings]")
{
    Settings settings;
    const QSignalSpy spy(&settings, &Settings::DisplaySettingsChanged);

    settings.SetApertureFloorDecibels(-40.0f);
    settings.SetApertureCeilingDecibels(20.0f);
    REQUIRE(spy.count() == 2);
    REQUIRE(settings.GetApertureFloorDecibels() == -40.0f);
    REQUIRE(settings.GetApertureCeilingDecibels() == 20.0f);
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
