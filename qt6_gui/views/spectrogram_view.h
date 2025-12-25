#pragma once

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
    /**
     * @brief Constructor
     * @param aController Reference to spectrogram controller
     * @param parent Qt parent widget (optional)
     */
    explicit SpectrogramView(SpectrogramController& aController, QWidget* parent = nullptr);
    ~SpectrogramView() override = default;

    /**
     * @brief Generate spectrogram image for given dimensions
     * @param aWidth Width in pixels
     * @param aHeight Height in pixels
     * @return Generated spectrogram image
     */
    QImage GenerateSpectrogramImage(size_t aWidth, size_t aHeight);

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    SpectrogramController& mController;
};
