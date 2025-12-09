#ifndef SPECTROGRAM_VIEW_H
#define SPECTROGRAM_VIEW_H

#include <QWidget>

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
    explicit SpectrogramView(QWidget* parent = nullptr);
    ~SpectrogramView() override = default;

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    // Future: Spectrogram data buffer, color map settings, etc.
};

#endif // SPECTROGRAM_VIEW_H
