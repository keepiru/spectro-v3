// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include <QPixmap>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

class ColorMap
{

  public:
    static constexpr size_t KLUTSize = 256;

    /// @brief Lightweight RGB color representation for LUT
    ///
    /// Used in the color map lookup table (LUT) to represent colors as raw 8-bit
    /// RGB values. This avoids repeated qRgb() calls in the hot path when
    /// rendering the spectrogram.
    struct Entry
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    using LUT = std::array<Entry, KLUTSize>;

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

    /// @brief Get the LUT for the specified color map type
    /// @param aType The color map type
    /// @return The corresponding LUT
    /// @throws std::invalid_argument if the type is unsupported
    [[nodiscard]] static LUT GetLUT(Type aType);

    /// @brief Generate a preview icon for a color map type
    /// @param aType Color map type to generate preview for
    /// @return QPixmap containing the color map preview
    ///
    /// Creates a horizontal gradient showing the color map's appearance.
    /// Used for display in UI combo boxes.
    [[nodiscard]] static QPixmap GeneratePreview(Type aType);

  private:
    /// @brief Generate a gradient LUT with specified RGB channels enabled
    /// @param enableRed Whether to enable the red channel
    /// @param enableGreen Whether to enable the green channel
    /// @param enableBlue Whether to enable the blue channel
    /// @return The generated gradient LUT
    /// @note This is a helper for GetLUT
    [[nodiscard]] static LUT GenerateGradientLUT(bool enableRed, bool enableGreen, bool enableBlue);
};
