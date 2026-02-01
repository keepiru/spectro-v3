// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "models/settings.h"
#include "include/colormap_data.h"
#include "include/global_constants.h"
#include "models/colormap.h"
#include <QObject>
#include <algorithm>
#include <array>
#include <audio_types.h>
#include <cstddef>
#include <cstdint>
#include <fft_window.h>
#include <stdexcept>

Settings::Settings(QObject* aParent)
  : QObject(aParent)
  , mColorMapLUTs()
{
    // Initialize default color maps
    for (ChannelCount ch = 0; ch < GKMaxChannels; ch++) {
        SetColorMap(ch, KDefaultColorMaps.at(ch));
    }
}

void
Settings::SetFFTSettings(const FFTSize aTransformSize, const FFTWindow::Type aWindowType)
{
    if (mFFTSize != aTransformSize || mWindowType != aWindowType) {
        mFFTSize = aTransformSize;
        mWindowType = aWindowType;
        emit FFTSettingsChanged();
        emit DisplaySettingsChanged();
    }
}

void
Settings::SetWindowScale(const WindowScale aScale)
{
    if (std::ranges::find(KValidWindowScales, aScale) == KValidWindowScales.end()) {
        throw std::invalid_argument("Invalid window scale");
    }

    mWindowScale = aScale;
    emit DisplaySettingsChanged();
}

void
Settings::SetColorMap(ChannelCount aChannel, ColorMap::Type aType)
{
    // Helper to set a gradient color map
    auto setGradientColorMap = [this, aChannel](bool enableRed, bool enableGreen, bool enableBlue) {
        for (size_t i = 0; i < KColorMapLUTSize; i++) {
            const auto intensity = static_cast<uint8_t>(i);
            mColorMapLUTs.at(aChannel).at(i) =
              ColorMap::Entry{ .r = enableRed ? intensity : uint8_t{ 0 },
                               .g = enableGreen ? intensity : uint8_t{ 0 },
                               .b = enableBlue ? intensity : uint8_t{ 0 } };
        }
    };

    switch (aType) {
        case ColorMap::Type::Disabled:
            // Disabled map is all black
            setGradientColorMap(false, false, false);
            break;
        case ColorMap::Type::White:
            setGradientColorMap(true, true, true);
            break;
        case ColorMap::Type::Red:
            setGradientColorMap(true, false, false);
            break;
        case ColorMap::Type::Green:
            setGradientColorMap(false, true, false);
            break;
        case ColorMap::Type::Blue:
            setGradientColorMap(false, false, true);
            break;
        case ColorMap::Type::Cyan:
            setGradientColorMap(false, true, true);
            break;
        case ColorMap::Type::Magenta:
            setGradientColorMap(true, false, true);
            break;
        case ColorMap::Type::Yellow:
            setGradientColorMap(true, true, false);
            break;
        case ColorMap::Type::Viridis:
            mColorMapLUTs.at(aChannel) = GKViridisLUT;
            break;
        case ColorMap::Type::Plasma:
            mColorMapLUTs.at(aChannel) = GKPlasmaLUT;
            break;
        case ColorMap::Type::Inferno:
            mColorMapLUTs.at(aChannel) = GKInfernoLUT;
            break;
        case ColorMap::Type::Magma:
            mColorMapLUTs.at(aChannel) = GKMagmaLUT;
            break;
        case ColorMap::Type::Turbo:
            mColorMapLUTs.at(aChannel) = GKTurboLUT;
            break;
        case ColorMap::Type::Cividis:
            mColorMapLUTs.at(aChannel) = GKCividisLUT;
            break;
        case ColorMap::Type::Hot:
            mColorMapLUTs.at(aChannel) = GKHotLUT;
            break;
        case ColorMap::Type::Cool:
            mColorMapLUTs.at(aChannel) = GKCoolLUT;
            break;
        case ColorMap::Type::Twilight:
            mColorMapLUTs.at(aChannel) = GKTwilightLUT;
            break;
        case ColorMap::Type::Seismic:
            mColorMapLUTs.at(aChannel) = GKSeismicLUT;
            break;
        case ColorMap::Type::Jet:
            mColorMapLUTs.at(aChannel) = GKJetLUT;
            break;
        default:
            throw std::invalid_argument("Settings::SetColorMap: Unsupported color map type");
    }

    mSelectedColorMaps.at(aChannel) = aType;
    emit DisplaySettingsChanged();
}

void
Settings::SetApertureFloorDecibels(const float aFloorDecibels)
{
    mApertureFloorDecibels = aFloorDecibels;
    emit DisplaySettingsChanged();
}

void
Settings::SetApertureCeilingDecibels(const float aCeilingDecibels)
{
    mApertureCeilingDecibels = aCeilingDecibels;
    emit DisplaySettingsChanged();
}
