#pragma once

#include "include/global_constants.h"
#include <QWidget>
#include <array>
#include <cstdint>

// Forward declarations
class SpectrogramController;

/**
 * @brief Waterfall spectrogram display widget
 *
 * Displays a scrolling waterfall plot of the spectrogram with frequency on the
 * horizontal axis and time on the vertical axis. Colors represent magnitude.
 *
 * Future features:
 * - Scrolling/scrubbing through time
 * - Configurable color maps (viridis, plasma, grayscale)
 * - Frequency and time axis labels
 * - dB scale display
 * - Zoom/pan controls
 */
class SpectrogramView : public QWidget
{
    Q_OBJECT

  public:
    enum class ColorMapType
    {
        kWhite,
        kRed,
        kGreen,
        kBlue,
        kCyan,
        kMagenta,
        kYellow,
        kViridis,
        kPlasma,
        kInferno,
        kMagma,
    };

    // Default color maps for each channel.
    static constexpr std::array<ColorMapType, gkMaxChannels> kDefaultColorMaps = {
        ColorMapType::kCyan,  ColorMapType::kRed,   ColorMapType::kWhite,
        ColorMapType::kWhite, ColorMapType::kWhite, ColorMapType::kWhite,
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

    /**
     * @brief Constructor
     * @param aController Pointer to spectrogram controller (must not be null)
     * @param parent Qt parent widget (optional)
     */
    explicit SpectrogramView(SpectrogramController* aController, QWidget* parent = nullptr);
    ~SpectrogramView() override = default;

    /**
     * @brief Set the color map type
     * @param aChannel Channel index (0-based)
     * @param aType Color map type
     */
    void SetColorMap(size_t aChannel, ColorMapType aType);

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
        return mColorMapLUT.at(aChannel).at(aIndex);
    }

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    static inline constexpr size_t kColorMapLUTSize = 256;

    SpectrogramController* mController; // Not owned

    // Precomputed color maps for magnitude to color mapping.  The simple
    // nested array structure provides fast access in the hot path.
    std::array<std::array<ColorMapEntry, kColorMapLUTSize>, gkMaxChannels> mColorMapLUT;
};
