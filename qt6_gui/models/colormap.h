// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

class ColorMap
{

  public:
    enum class Type : uint8_t
    {
        Disabled,
        White,
        Red,
        Green,
        Blue,
        Cyan,
        Magenta,
        Yellow,
        Viridis,
        Plasma,
        Inferno,
        Magma,
        Turbo,
        Cividis,
        Hot,
        Cool,
        Twilight,
        Seismic,
        Jet,
        Count,
    };

    /// @brief Mapping of Type to string names
    /// Used for the UI pulldown to select color maps.
    static constexpr std::array<std::pair<Type, std::string_view>, static_cast<size_t>(Type::Count)>
      TypeNames = { { { Type::Disabled, "Disabled" },
                      { Type::White, "White" },
                      { Type::Red, "Red" },
                      { Type::Green, "Green" },
                      { Type::Blue, "Blue" },
                      { Type::Cyan, "Cyan" },
                      { Type::Magenta, "Magenta" },
                      { Type::Yellow, "Yellow" },
                      { Type::Viridis, "Viridis" },
                      { Type::Plasma, "Plasma" },
                      { Type::Inferno, "Inferno" },
                      { Type::Magma, "Magma" },
                      { Type::Turbo, "Turbo" },
                      { Type::Cividis, "Cividis" },
                      { Type::Hot, "Hot" },
                      { Type::Cool, "Cool" },
                      { Type::Twilight, "Twilight" },
                      { Type::Seismic, "Seismic" },
                      { Type::Jet, "Jet" } } };
};
