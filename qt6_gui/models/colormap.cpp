// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "colormap.h"
#include "include/colormap_data.h"
#include <cstddef>
#include <cstdint>
#include <stdexcept>

ColorMap::LUT
ColorMap::GenerateGradientLUT(bool enableRed, bool enableGreen, bool enableBlue)
{
    ColorMap::LUT lut;

    for (size_t i = 0; i < ColorMap::KLUTSize; i++) {
        const auto intensity = static_cast<uint8_t>(i);
        lut.at(i) = ColorMap::Entry{ .r = enableRed ? intensity : uint8_t{ 0 },
                                     .g = enableGreen ? intensity : uint8_t{ 0 },
                                     .b = enableBlue ? intensity : uint8_t{ 0 } };
    }

    return lut;
}

ColorMap::LUT
ColorMap::GetLUT(ColorMap::Type aType)
{
    switch (aType) {
        case ColorMap::Type::Disabled:
            // Disabled map is all black
            return GenerateGradientLUT(false, false, false);
        case ColorMap::Type::White:
            return GenerateGradientLUT(true, true, true);
        case ColorMap::Type::Red:
            return GenerateGradientLUT(true, false, false);
        case ColorMap::Type::Green:
            return GenerateGradientLUT(false, true, false);
        case ColorMap::Type::Blue:
            return GenerateGradientLUT(false, false, true);
        case ColorMap::Type::Cyan:
            return GenerateGradientLUT(false, true, true);
        case ColorMap::Type::Magenta:
            return GenerateGradientLUT(true, false, true);
        case ColorMap::Type::Yellow:
            return GenerateGradientLUT(true, true, false);
        case ColorMap::Type::Viridis:
            return GKViridisLUT;
        case ColorMap::Type::Plasma:
            return GKPlasmaLUT;
        case ColorMap::Type::Inferno:
            return GKInfernoLUT;
        case ColorMap::Type::Magma:
            return GKMagmaLUT;
        case ColorMap::Type::Turbo:
            return GKTurboLUT;
        case ColorMap::Type::Cividis:
            return GKCividisLUT;
        case ColorMap::Type::Hot:
            return GKHotLUT;
        case ColorMap::Type::Cool:
            return GKCoolLUT;
        case ColorMap::Type::Twilight:
            return GKTwilightLUT;
        case ColorMap::Type::Seismic:
            return GKSeismicLUT;
        case ColorMap::Type::Jet:
            return GKJetLUT;
        default:
            throw std::invalid_argument("Unsupported color map type");
    }
}