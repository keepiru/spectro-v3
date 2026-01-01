#pragma once

#include "include/global_constants.h"
#include <QObject>
#include <array>
#include <audio_types.h>
#include <cstddef>
#include <fft_window.h>
#include <string_view>

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

    // Linter complains because of the Last sentinel.  Ignore that.
    // NOLINTNEXTLINE(readability-enum-initial-value)
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
        Count,
    };

    /**
     *  @brief Mapping of ColorMapType to string names
     *  Used for the UI pulldown to select color maps.
     */
    static constexpr std::array<std::pair<ColorMapType, std::string_view>,
                                static_cast<size_t>(ColorMapType::Count)>
      KColorMapTypeNames = { { { ColorMapType::White, "White" },
                               { ColorMapType::Red, "Red" },
                               { ColorMapType::Green, "Green" },
                               { ColorMapType::Blue, "Blue" },
                               { ColorMapType::Cyan, "Cyan" },
                               { ColorMapType::Magenta, "Magenta" },
                               { ColorMapType::Yellow, "Yellow" },
                               { ColorMapType::Viridis, "Viridis" },
                               { ColorMapType::Plasma, "Plasma" },
                               { ColorMapType::Inferno, "Inferno" },
                               { ColorMapType::Magma, "Magma" } } };

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

    using ColorMapLUTs = std::array<std::array<ColorMapEntry, KColorMapLUTSize>, GKMaxChannels>;

    explicit Settings(QObject* aParent = nullptr);

    /**
     ** FFT Settings
     **/

    /**
     * @brief Set FFT settings
     * @param aTransformSize New FFT size (must be power of 2)
     * @param aWindowType New window function type
     * @throws std::invalid_argument if aTransformSize is not positive
     *
     * These are set together because changing either requires recreating
     * FFT and window objects.
     */
    void SetFFTSettings(FFTSize aTransformSize, FFTWindow::Type aWindowType);

    /**
     * @brief Get the FFT size
     * @return Current FFT size
     */
    [[nodiscard]] FFTSize GetFFTSize() const { return mFFTSize; }

    /**
     * @brief Get the window function type
     * @return Current window function type
     */
    [[nodiscard]] FFTWindow::Type GetWindowType() const { return mWindowType; }

    /**
     ** Window scale and stride
     **/

    /**
     * @brief Set the window scale
     * @param aScale New window scale (1, 2, 4, 8, or 16)
     * @throws std::invalid_argument if aScale is not valid
     */
    void SetWindowScale(size_t aScale);

    /**
     * @brief Get the window scale
     * @return Current window scale
     */
    [[nodiscard]] size_t GetWindowScale() const { return mWindowScale; }

    /**
     * @brief Get the window stride based on current FFT size and window scale
     * @return Current window stride (FFT size / window scale)
     */
    [[nodiscard]] FFTSize GetWindowStride() const
    {
        return mFFTSize / static_cast<FFTSize>(mWindowScale);
    }

    /**
     ** Display settings - Aperture (decibel range)
     **/

    /**
     * @brief Set the aperture minimum decibel value
     * @param aMinDecibels New minimum decibel value
     */
    void SetApertureMinDecibels(float aMinDecibels);

    /**
     * @brief Set the aperture maximum decibel value
     * @param aMaxDecibels New maximum decibel value
     */
    void SetApertureMaxDecibels(float aMaxDecibels);

    /*
     * @brief Get the minimum decibel value of the aperture
     * @return Minimum decibel value
     */
    [[nodiscard]] float GetApertureMinDecibels() const { return mApertureMinDecibels; }

    /**
     * @brief Get the maximum decibel value of the aperture
     * @return Maximum decibel value
     */
    [[nodiscard]] float GetApertureMaxDecibels() const { return mApertureMaxDecibels; }

    /**
     ** Color map settings
     **/

    /**
     * @brief Get the color map LUTs for all channels
     * @return Reference to the array of color map LUTs
     */
    [[nodiscard]] const ColorMapLUTs& GetColorMapLUTs() const { return mColorMapLUTs; }

    /**
     * @brief Get the color map LUT value at a specific index
     * @param aChannel Channel index (0-based)
     * @param aIndex Index into the LUT (0-255)
     * @return RGB color value
     *
     * This is used to test LUT generation.  Prod code accesses the array
     * directly for performance.
     */
    [[nodiscard]] ColorMapEntry GetColorMapValue(ChannelCount aChannel, uint8_t aIndex) const
    {
        return mColorMapLUTs.at(aChannel).at(aIndex);
    }

    /**
     * @brief Set the color map type
     * @param aChannel Channel index (0-based)
     * @param aType Color map type
     */
    void SetColorMap(ChannelCount aChannel, ColorMapType aType);

    /**
     * @brief Get the color map type for a channel
     * @param aChannel Channel index (0-based)
     * @return Current color map type
     */
    [[nodiscard]] ColorMapType GetColorMap(ChannelCount aChannel) const
    {
        return mSelectedColorMaps.at(aChannel);
    }

  signals:
    /**
     * @brief Emitted when FFT size or window type changes
     *
     * Listeners (SpectrogramController) should recreate FFT/window objects.
     */
    void FFTSettingsChanged();

    /**
     * @brief Emitted when display-related settings change
     *
     * Listeners (SpectrogramView, SpectrumPlot) should redraw
     */
    void DisplaySettingsChanged();

  private:
    static constexpr FFTSize KDefaultFFTSize = 2048;
    FFTSize mFFTSize = KDefaultFFTSize;
    FFTWindow::Type mWindowType = FFTWindow::Type::Hann;

    // The window scale is the divisor applied to the FFT size to determine
    // the hop size (stride) between successive FFT windows.
    static constexpr size_t KDefaultWindowScale = 2;
    size_t mWindowScale = KDefaultWindowScale;

    // The aperture is the visible decibel range in the spectrogram and spectrum
    // plot.
    static constexpr float KDefaultApertureMinDecibels = -30.0f;
    static constexpr float KDefaultApertureMaxDecibels = 30.0f;
    float mApertureMinDecibels = KDefaultApertureMinDecibels;
    float mApertureMaxDecibels = KDefaultApertureMaxDecibels;

    // Default color maps for each channel.
    static constexpr std::array<ColorMapType, GKMaxChannels> KDefaultColorMaps = {
        ColorMapType::Magenta, ColorMapType::Green, ColorMapType::White,
        ColorMapType::White,   ColorMapType::White, ColorMapType::White,
    };

    // Color map lookup tables (LUTs) for each channel.  The simple nested array
    // structure provides fast access in the hot path.
    ColorMapLUTs mColorMapLUTs;

    // Selected color maps for each channel.
    std::array<ColorMapType, GKMaxChannels> mSelectedColorMaps = KDefaultColorMaps;
};
