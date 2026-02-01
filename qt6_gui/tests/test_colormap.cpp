// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "models/colormap.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("ColorMap can be instantiated", "[colormap]")
{
    ColorMap colorMap;
    REQUIRE(true);
}
