// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "models/settings.h"
#include "include/global_constants.h"
#include "models/colormap.h"
#include <QObject>
#include <algorithm>
#include <array>
#include <audio_types.h>
#include <fft_window.h>
#include <stdexcept>

Settings::Settings(QObject* aParent)
  : QObject(aParent)
  , mColorMapLUTs()
{
    // Initialize default color maps
    for (ChannelCount ch = 0; ch < GKMaxChannels; ch++) {
        SetColorMapType(ch, KDefaultColorMaps.at(ch));
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
Settings::SetColorMapType(ChannelCount aChannel, ColorMap::Type aType)
{
    mColorMapLUTs.at(aChannel) = ColorMap::GetLUT(aType);
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