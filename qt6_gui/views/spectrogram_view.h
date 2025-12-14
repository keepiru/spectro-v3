#ifndef SPECTROGRAM_VIEW_H
#define SPECTROGRAM_VIEW_H

#include <array>
#include <QRgb>
#include <QWidget>

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
        kGrayscale,
        kViridis,
        kPlasma,
        kInferno,
        kMagma,
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
     * @param aType Color map type
     */
    void SetColorMap(ColorMapType aType);

    /**
     * @brief Get the color map LUT value at a specific index
     * @param aIndex Index into the LUT (0-255)
     * @return RGB color value
     *
     * This is used to test LUT generation.  Prod code accesses the array
     * directly for performance.
     */
    [[nodiscard]] QRgb GetColorMapValue(uint8_t aIndex) const { return mColorMapLUT[aIndex]; }

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    SpectrogramController* mController; // Not owned

    std::array<QRgb, 256> mColorMapLUT; // Precomputed color map for magnitude to color mapping
};

#endif // SPECTROGRAM_VIEW_H
