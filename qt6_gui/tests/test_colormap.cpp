// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "models/colormap.h"
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

TEST_CASE("ColorMap::GetLUT invalid type", "[colormap]")
{
    // Invalid enum value (not in defined range)
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    const auto invalidValue = static_cast<ColorMap::Type>(999);
    REQUIRE_THROWS_AS(ColorMap::GetLUT(invalidValue), std::invalid_argument);

    // Also verify the Count sentinel is not accepted
    REQUIRE_THROWS_AS(ColorMap::GetLUT(ColorMap::Type::Count), std::invalid_argument);
}

TEST_CASE("ColorMap::GetLUT valid types return without throwing", "[colormap]")
{
    for (size_t i = 0; i < static_cast<size_t>(ColorMap::Type::Count); i++) {
        const auto type = static_cast<ColorMap::Type>(i);
        REQUIRE_NOTHROW(ColorMap::GetLUT(type));
    }
}

TEST_CASE("ColorMap::GetLUT Disabled is all black", "[colormap]")
{
    const auto lut = ColorMap::GetLUT(ColorMap::Type::Disabled);
    for (size_t i = 0; i < ColorMap::KLUTSize; i++) {
        const auto entry = lut.at(i);
        REQUIRE(entry.r == 0);
        REQUIRE(entry.g == 0);
        REQUIRE(entry.b == 0);
    }
}

TEST_CASE("ColorMap::GetLUT White is white gradient", "[colormap]")
{
    const auto lut = ColorMap::GetLUT(ColorMap::Type::White);
    for (size_t i = 0; i < ColorMap::KLUTSize; i++) {
        const auto entry = lut.at(i);
        const auto intensity = static_cast<uint8_t>(i);
        REQUIRE(entry.r == intensity);
        REQUIRE(entry.g == intensity);
        REQUIRE(entry.b == intensity);
    }
}

TEST_CASE("ColorMap::GetLUT Red is red gradient", "[colormap]")
{
    const auto lut = ColorMap::GetLUT(ColorMap::Type::Red);
    for (size_t i = 0; i < ColorMap::KLUTSize; i++) {
        const auto entry = lut.at(i);
        const auto intensity = static_cast<uint8_t>(i);
        REQUIRE(entry.r == intensity);
        REQUIRE(entry.g == 0);
        REQUIRE(entry.b == 0);
    }
}

TEST_CASE("ColorMap::GetLUT Viridis colormap has expected RGB values", "[colormap]")
{
    const auto lut = ColorMap::GetLUT(ColorMap::Type::Viridis);
    const auto entry0 = lut.at(0);
    const auto entry128 = lut.at(128);
    const auto entry255 = lut.at(255);

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

TEST_CASE("ColorMap::GetLUT Plasma colormap has expected RGB values", "[colormap]")
{
    const auto lut = ColorMap::GetLUT(ColorMap::Type::Plasma);
    const auto entry0 = lut.at(0);
    const auto entry128 = lut.at(128);
    const auto entry255 = lut.at(255);

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

TEST_CASE("ColorMap::GetLUT Inferno colormap has expected RGB values", "[colormap]")
{
    const auto lut = ColorMap::GetLUT(ColorMap::Type::Inferno);
    const auto entry0 = lut.at(0);
    const auto entry128 = lut.at(128);
    const auto entry255 = lut.at(255);

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

TEST_CASE("ColorMap::GetLUT Magma colormap has expected RGB values", "[colormap]")
{
    const auto lut = ColorMap::GetLUT(ColorMap::Type::Magma);
    const auto entry0 = lut.at(0);
    const auto entry128 = lut.at(128);
    const auto entry255 = lut.at(255);

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

TEST_CASE("ColorMap::GetLUT Turbo colormap has expected RGB values", "[colormap]")
{
    const auto lut = ColorMap::GetLUT(ColorMap::Type::Turbo);
    const auto entry0 = lut.at(0);
    const auto entry128 = lut.at(128);
    const auto entry255 = lut.at(255);

    // Turbo: dark purple, bright green-yellow, dark red
    REQUIRE(entry0.r == 48);
    REQUIRE(entry0.g == 18);
    REQUIRE(entry0.b == 59);
    REQUIRE(entry128.r == 164);
    REQUIRE(entry128.g == 252);
    REQUIRE(entry128.b == 60);
    REQUIRE(entry255.r == 122);
    REQUIRE(entry255.g == 4);
    REQUIRE(entry255.b == 3);
}

TEST_CASE("ColorMap::GetLUT Cividis colormap has expected RGB values", "[colormap]")
{
    const auto lut = ColorMap::GetLUT(ColorMap::Type::Cividis);
    const auto entry0 = lut.at(0);
    const auto entry128 = lut.at(128);
    const auto entry255 = lut.at(255);

    // Cividis: dark blue, gray, bright yellow
    REQUIRE(entry0.r == 0);
    REQUIRE(entry0.g == 34);
    REQUIRE(entry0.b == 78);
    REQUIRE(entry128.r == 125);
    REQUIRE(entry128.g == 124);
    REQUIRE(entry128.b == 120);
    REQUIRE(entry255.r == 254);
    REQUIRE(entry255.g == 232);
    REQUIRE(entry255.b == 56);
}

TEST_CASE("ColorMap::GetLUT Hot colormap has expected RGB values", "[colormap]")
{
    const auto lut = ColorMap::GetLUT(ColorMap::Type::Hot);
    const auto entry0 = lut.at(0);
    const auto entry128 = lut.at(128);
    const auto entry255 = lut.at(255);

    // Hot: black, orange, white
    REQUIRE(entry0.r == 11);
    REQUIRE(entry0.g == 0);
    REQUIRE(entry0.b == 0);
    REQUIRE(entry128.r == 255);
    REQUIRE(entry128.g == 92);
    REQUIRE(entry128.b == 0);
    REQUIRE(entry255.r == 255);
    REQUIRE(entry255.g == 255);
    REQUIRE(entry255.b == 255);
}

TEST_CASE("ColorMap::GetLUT Cool colormap has expected RGB values", "[colormap]")
{
    const auto lut = ColorMap::GetLUT(ColorMap::Type::Cool);
    const auto entry0 = lut.at(0);
    const auto entry128 = lut.at(128);
    const auto entry255 = lut.at(255);

    // Cool: cyan, blue-magenta, magenta
    REQUIRE(entry0.r == 0);
    REQUIRE(entry0.g == 255);
    REQUIRE(entry0.b == 255);
    REQUIRE(entry128.r == 128);
    REQUIRE(entry128.g == 127);
    REQUIRE(entry128.b == 255);
    REQUIRE(entry255.r == 255);
    REQUIRE(entry255.g == 0);
    REQUIRE(entry255.b == 255);
}

TEST_CASE("ColorMap::GetLUT Twilight colormap has expected RGB values", "[colormap]")
{
    const auto lut = ColorMap::GetLUT(ColorMap::Type::Twilight);
    const auto entry0 = lut.at(0);
    const auto entry128 = lut.at(128);
    const auto entry255 = lut.at(255);

    // Twilight: light purple, dark purple, light purple
    REQUIRE(entry0.r == 226);
    REQUIRE(entry0.g == 217);
    REQUIRE(entry0.b == 226);
    REQUIRE(entry128.r == 48);
    REQUIRE(entry128.g == 20);
    REQUIRE(entry128.b == 55);
    REQUIRE(entry255.r == 226);
    REQUIRE(entry255.g == 217);
    REQUIRE(entry255.b == 226);
}

TEST_CASE("ColorMap::GetLUT Seismic colormap has expected RGB values", "[colormap]")
{
    const auto lut = ColorMap::GetLUT(ColorMap::Type::Seismic);
    const auto entry0 = lut.at(0);
    const auto entry128 = lut.at(128);
    const auto entry255 = lut.at(255);

    // Seismic: dark blue, white, dark red
    REQUIRE(entry0.r == 0);
    REQUIRE(entry0.g == 0);
    REQUIRE(entry0.b == 76);
    REQUIRE(entry128.r == 255);
    REQUIRE(entry128.g == 253);
    REQUIRE(entry128.b == 253);
    REQUIRE(entry255.r == 128);
    REQUIRE(entry255.g == 0);
    REQUIRE(entry255.b == 0);
}

TEST_CASE("ColorMap::GetLUT Jet colormap has expected RGB values", "[colormap]")
{
    const auto lut = ColorMap::GetLUT(ColorMap::Type::Jet);
    const auto entry0 = lut.at(0);
    const auto entry128 = lut.at(128);
    const auto entry255 = lut.at(255);

    // Jet: dark blue, cyan-green, dark red
    REQUIRE(entry0.r == 0);
    REQUIRE(entry0.g == 0);
    REQUIRE(entry0.b == 128);
    REQUIRE(entry128.r == 125);
    REQUIRE(entry128.g == 255);
    REQUIRE(entry128.b == 122);
    REQUIRE(entry255.r == 128);
    REQUIRE(entry255.g == 0);
    REQUIRE(entry255.b == 0);
}