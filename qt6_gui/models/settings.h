// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include "include/global_constants.h"
#include "models/colormap.h"
#include <QObject>
#include <array>
#include <audio_types.h>
#include <cstddef>
#include <cstdint>
#include <fft_window.h>
#include <utility>

// Forward declarations
class SpectrogramView;

/// @brief Store runtime settings for the application.
///
/// Owns all configurable application settings. Emits signals when settings
/// change. Components initialize from Settings and listen for changes.
class Settings : public QObject
{
    Q_OBJECT

  public:
    static constexpr size_t KColorMapLUTSize = 256;
    static constexpr std::array<WindowScale, 5> KValidWindowScales{ 1, 2, 4, 8, 16 };
    static constexpr std::array<FFTSize, 6> KValidFFTSizes{ 512, 1024, 2048, 4096, 8192, 16384 };
    static constexpr std::pair<int16_t, int16_t> KApertureLimitsDecibels = { -80, 100 };

    /// @brief Lightweight RGB color representation for LUT
    ///
    /// Used in the color map lookup table (LUT) to represent colors as raw 8-bit
    /// RGB values. This avoids repeated qRgb() calls in the hot path when
    /// rendering the spectrogram.
    struct ColorMapEntry
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    using ColorMapLUTs = std::array<std::array<ColorMapEntry, KColorMapLUTSize>, GKMaxChannels>;

    explicit Settings(QObject* aParent = nullptr);

    ///
    /// FFT Settings
    ///

    /// @brief Set FFT settings
    /// @param aTransformSize New FFT size (must be power of 2)
    /// @param aWindowType New window function type
    /// @throws std::invalid_argument if aTransformSize is not positive
    ///
    /// These are set together because changing either requires recreating
    /// FFT and window objects.
    void SetFFTSettings(FFTSize aTransformSize, FFTWindow::Type aWindowType);

    /// @brief Get the FFT size
    /// @return Current FFT size
    [[nodiscard]] FFTSize GetFFTSize() const { return mFFTSize; }

    /// @brief Get the window function type
    /// @return Current window function type
    [[nodiscard]] FFTWindow::Type GetWindowType() const { return mWindowType; }

    ///
    /// Window scale and stride
    ///

    /// @brief Set the window scale
    /// @param aScale New window scale (1, 2, 4, 8, or 16)
    /// @throws std::invalid_argument if aScale is not valid
    void SetWindowScale(WindowScale aScale);

    /// @brief Get the window scale
    /// @return Current window scale
    [[nodiscard]] WindowScale GetWindowScale() const { return mWindowScale; }

    /// @brief Get the window stride based on current FFT size and window scale
    /// @return Current window stride (FFT size / window scale)
    [[nodiscard]] FFTSize GetWindowStride() const { return mFFTSize / mWindowScale; }

    ///
    /// Display settings - Aperture (decibel range)
    ///

    /// @brief Set the aperture floor decibel value
    /// @param aFloorDecibels New floor decibel value
    /// @note May be greater than ceiling decibel value, resulting in inverted display
    void SetApertureFloorDecibels(float aFloorDecibels);

    /// @brief Set the aperture ceiling decibel value
    /// @param aCeilingDecibels New ceiling decibel value
    /// @note May be less than floor decibel value, resulting in inverted display
    void SetApertureCeilingDecibels(float aCeilingDecibels);

    /// @brief Get the floor decibel value of the aperture
    /// @return Floor decibel value
    [[nodiscard]] float GetApertureFloorDecibels() const { return mApertureFloorDecibels; }

    /// @brief Get the ceiling decibel value of the aperture
    /// @return Ceiling decibel value
    [[nodiscard]] float GetApertureCeilingDecibels() const { return mApertureCeilingDecibels; }

    ///
    /// Color map settings
    ///

    /// @brief Get the color map LUTs for all channels
    /// @return Reference to the array of color map LUTs
    [[nodiscard]] const ColorMapLUTs& GetColorMapLUTs() const { return mColorMapLUTs; }

    /// @brief Get the color map LUT value at a specific index
    /// @param aChannel Channel index (0-based)
    /// @param aIndex Index into the LUT (0-255)
    /// @return RGB color value
    ///
    /// This is used to test LUT generation.  Prod code accesses the array
    /// directly for performance.
    [[nodiscard]] ColorMapEntry GetColorMapValue(ChannelCount aChannel, uint8_t aIndex) const
    {
        return mColorMapLUTs.at(aChannel).at(aIndex);
    }

    /// @brief Set the color map type
    /// @param aChannel Channel index (0-based)
    /// @param aType Color map type
    void SetColorMap(ChannelCount aChannel, ColorMap::Type aType);

    /// @brief Get the color map type for a channel
    /// @param aChannel Channel index (0-based)
    /// @return Current color map type
    [[nodiscard]] ColorMap::Type GetColorMap(ChannelCount aChannel) const
    {
        return mSelectedColorMaps.at(aChannel);
    }

    /// @brief Get whether live mode is enabled
    /// @return True if live mode is enabled, false otherwise
    [[nodiscard]] bool IsLiveMode() const { return mIsLiveMode; }

    /// @brief Set whether live mode is enabled
    /// @param aIsLiveMode True to enable live mode, false to disable
    void SetLiveMode(const bool aIsLiveMode) { mIsLiveMode = aIsLiveMode; }

    /// @brief Clear live mode (set to false)
    /// @note This slot connects with the scrollbar's actionTriggered signal
    void ClearLiveMode() { mIsLiveMode = false; }

  signals:
    /// @brief Emitted when FFT size or window type changes
    ///
    /// Listeners (SpectrogramController) should recreate FFT/window objects.
    void FFTSettingsChanged();

    /// @brief Emitted when display-related settings change
    ///
    /// Listeners (SpectrogramView, SpectrumPlot) should redraw
    void DisplaySettingsChanged();

  private:
    static constexpr FFTSize KDefaultFFTSize = 2048;
    FFTSize mFFTSize = KDefaultFFTSize;
    FFTWindow::Type mWindowType = FFTWindow::Type::Hann;

    // The window scale is the divisor applied to the FFT size to determine
    // the hop size (stride) between successive FFT windows.
    static constexpr WindowScale KDefaultWindowScale = 2;
    WindowScale mWindowScale = KDefaultWindowScale;

    // The aperture is the visible decibel range in the spectrogram and spectrum
    // plot.
    static constexpr float KDefaultApertureFloorDecibels = -20.0f;
    static constexpr float KDefaultApertureCeilingDecibels = 40.0f;
    float mApertureFloorDecibels = KDefaultApertureFloorDecibels;
    float mApertureCeilingDecibels = KDefaultApertureCeilingDecibels;

    // Default color maps for each channel.
    static constexpr std::array<ColorMap::Type, GKMaxChannels> KDefaultColorMaps = {
        ColorMap::Type::Magenta, ColorMap::Type::Green, ColorMap::Type::White,
        ColorMap::Type::White,   ColorMap::Type::White, ColorMap::Type::White,
    };

    // Color map lookup tables (LUTs) for each channel.  The simple nested array
    // structure provides fast access in the hot path.
    ColorMapLUTs mColorMapLUTs;

    // Selected color maps for each channel.
    std::array<ColorMap::Type, GKMaxChannels> mSelectedColorMaps = KDefaultColorMaps;

    bool mIsLiveMode{ true }; ///< Whether we are following live audio or viewing history
};
