#pragma once

#include "controllers/spectrogram_controller.h"
#include <QWidget>

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
    explicit SpectrumPlot(SpectrogramController* aController, QWidget* parent = nullptr);
    ~SpectrumPlot() override = default;

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    SpectrogramController* mController; // Not owned
};
