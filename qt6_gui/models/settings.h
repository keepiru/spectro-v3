#pragma once

#include "include/global_constants.h"
#include <QObject>
#include <cstddef>
#include <fft_window.h>

// Forward declarations
class SpectrogramView;

/**
 * @brief Store runtime settings for the application.
 *
 * Owns all configurable application settings. Emits signals when settings
 * change. Components initialize from Settings and listen for changes.
 */
class Settings : public QObject
{
    Q_OBJECT

  public:
    static constexpr size_t KColorMapLUTSize = 256;

    enum class ColorMapType : uint8_t
    {
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
    };

    /**
     * @brief Lightweight RGB color representation for LUT
     *
     * Used in the color map lookup table (LUT) to represent colors as raw 8-bit
     * RGB values. This avoids repeated qRgb() calls in the hot path when
     * rendering the spectrogram.
     */
    struct ColorMapEntry
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    explicit Settings(QObject* aParent = nullptr);

    // FFT Settings
    [[nodiscard]] size_t GetFFTSize() const { return mFFTSize; }
    [[nodiscard]] FFTWindow::Type GetWindowType() const { return mWindowType; }
    [[nodiscard]] size_t GetWindowStride() const { return mWindowStride; }
    [[nodiscard]] float GetApertureMinDecibels() const { return mApertureMinDecibels; }
    [[nodiscard]] float GetApertureMaxDecibels() const { return mApertureMaxDecibels; }

    /**
     * @brief Set FFT settings
     * @param aTransformSize New FFT size (must be power of 2)
     * @param aWindowType New window function type
     * @throws std::invalid_argument if aTransformSize is zero
     *
     * These are set together because changing either requires recreating
     * FFT and window objects.
     */
    void SetFFTSettings(size_t aTransformSize, FFTWindow::Type aWindowType);
    void SetWindowStride(size_t aStride);

    [[nodiscard]] const std::array<std::array<ColorMapEntry, KColorMapLUTSize>, gkMaxChannels>&
    GetColorMapLUTs() const
    {
        return mColorMapLUTs;
    }

    /**
     * @brief Get the color map LUT value at a specific index
     * @param aChannel Channel index (0-based)
     * @param aIndex Index into the LUT (0-255)
     * @return RGB color value
     *
     * This is used to test LUT generation.  Prod code accesses the array
     * directly for performance.
     */
    [[nodiscard]] ColorMapEntry GetColorMapValue(size_t aChannel, uint8_t aIndex) const
    {
        return mColorMapLUTs.at(aChannel).at(aIndex);
    }

    /**
     * @brief Set the color map type
     * @param aChannel Channel index (0-based)
     * @param aType Color map type
     */
    void SetColorMap(size_t aChannel, ColorMapType aType);

  signals:
    /**
     * @brief Emitted when FFT size or window type changes
     *
     * Listeners (SpectrogramController) should recreate FFT/window objects.
     */
    void FFTSettingsChanged();

    /**
     * @brief Emitted when window stride changes
     *
     * Listeners (SpectrogramController) should update stride and snap view position.
     */
    void WindowStrideChanged();

  private:
    static constexpr size_t KDefaultFFTSize = 2048;
    size_t mFFTSize = KDefaultFFTSize;
    FFTWindow::Type mWindowType = FFTWindow::Type::Hann;
    static constexpr size_t KDefaultWindowStride = 1024;
    size_t mWindowStride = KDefaultWindowStride;

    // The aperture is the visible decibel range in the spectrogram and spectrum
    // plot.
    static constexpr float KDefaultApertureMinDecibels = -30.0f;
    static constexpr float KDefaultApertureMaxDecibels = 30.0f;
    float mApertureMinDecibels = KDefaultApertureMinDecibels;
    float mApertureMaxDecibels = KDefaultApertureMaxDecibels;

    // Default color maps for each channel.
    static constexpr std::array<ColorMapType, gkMaxChannels> KDefaultColorMaps = {
        ColorMapType::Cyan,  ColorMapType::Red,   ColorMapType::White,
        ColorMapType::White, ColorMapType::White, ColorMapType::White,
    };

    // Color map lookup tables (LUTs) for each channel.  The simple nested array
    // structure provides fast access in the hot path.
    std::array<std::array<ColorMapEntry, KColorMapLUTSize>, gkMaxChannels> mColorMapLUTs;
};
