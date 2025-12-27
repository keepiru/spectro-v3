#pragma once

#include <QWidget>

class SpectrogramController;

/**
 * @brief Real-time frequency spectrum line plot widget
 *
 * Displays a line plot of the current frequency spectrum with frequency on the
 * horizontal axis and magnitude on the vertical axis.
 *
 * Future features:
 * - Real-time spectrum line plot
 * - Frequency axis labels
 * - dB scale on vertical axis
 * - Peak markers
 * - Grid lines
 */
class SpectrumPlot : public QWidget
{
    Q_OBJECT

  public:
    explicit SpectrumPlot(const SpectrogramController& aController, QWidget* parent = nullptr);
    ~SpectrumPlot() override = default;

    /**
     * @brief Get the decibel values for the most recent stride in a given channel
     * @param aChannel Channel index (0-based)
     * @return std::vector<float> Vector of decibel values for the current FFT window
     * @throws std::out_of_range if aChannel is out of range
     */
    [[nodiscard]] std::vector<float> GetDecibels(size_t aChannel) const;

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    const SpectrogramController& mController;
};
