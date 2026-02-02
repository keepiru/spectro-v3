// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "controllers/settings_controller.h"
#include "models/settings.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("SettingsController constructor", "[settings_controller]")
{
    Settings settings;
    const SettingsController controller(settings);
}
