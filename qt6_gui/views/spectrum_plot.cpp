#include "spectrum_plot.h"
#include "controllers/spectrogram_controller.h"
#include <QPaintEvent>
#include <QPainter>
#include <QPolygonF>
#include <QWidget>
#include <Qt>
#include <QtTypes>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <dsp_utils.h>

SpectrumPlot::SpectrumPlot(SpectrogramController& aController, QWidget* parent)
  : QWidget(parent)
  , mController(aController)
{
    constexpr int kMinWidth = 400;
    constexpr int kMinHeight = 200;
    setMinimumSize(kMinWidth, kMinHeight);
}

void
SpectrumPlot::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), Qt::black);

    const auto kAvailableSampleCount = mController.GetAvailableSampleCount();
    const int64_t kTopSample = mController.CalculateTopSample(kAvailableSampleCount);
    const auto kChannels = mController.GetChannelCount();
    const auto kApertureMinDecibels = mController.GetSettings().GetApertureMinDecibels();
    const auto kApertureMaxDecibels = mController.GetSettings().GetApertureMaxDecibels();
    const auto kDecibelRange = kApertureMaxDecibels - kApertureMinDecibels;
    const auto kImplausiblySmallDecibelRange = 1e-6f;
    if (std::abs(kDecibelRange) < kImplausiblySmallDecibelRange) {
        // Avoid division by zero.  We can't draw anything if the range is zero.
        return;
    }
    const auto kInverseDecibelRange = 1.0f / kDecibelRange;

    for (size_t ch = 0; ch < kChannels; ch++) {
        const auto kRows = mController.GetRows(ch, kTopSample, 1);
        if (kRows.empty() || kRows[0].empty()) {
            continue;
        }

        const auto& kMagnitudes = kRows[0];
        const size_t kWidth = width();
        const size_t kHeight = height();

        switch (ch) {
            case 0:
                painter.setPen(Qt::magenta);
                break;
            case 1:
                painter.setPen(Qt::green);
                break;
            default:
                painter.setPen(Qt::white);
                break;
        }

        QPolygonF points;
        points.reserve(static_cast<qsizetype>(kMagnitudes.size()));

        const size_t kMaxX = std::min(kWidth, kMagnitudes.size());
        for (size_t x = 0; x < kMaxX; x++) { // NOLINT(readability-identifier-length)
            const float kDecibels = DSPUtils::MagnitudeToDecibels(kMagnitudes[x]);
            const float kNormalizedMagnitude =
              (kDecibels - kApertureMinDecibels) * kInverseDecibelRange;
            const float kYCoordinate =
              static_cast<float>(kHeight) - (kNormalizedMagnitude * static_cast<float>(kHeight));
            points.emplace_back(static_cast<float>(x), kYCoordinate);
        }

        painter.drawPolyline(points);
    }
}
