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
#include <vector>

SpectrumPlot::SpectrumPlot(const SpectrogramController& aController, QWidget* parent)
  : QWidget(parent)
  , mController(aController)
{
    constexpr int kMinWidth = 400;
    constexpr int kMinHeight = 200;
    setMinimumSize(kMinWidth, kMinHeight);
}

std::vector<float>
SpectrumPlot::GetDecibels(const size_t aChannel) const
{
    const int64_t kAvailableSampleCount = mController.GetAvailableSampleCount();
    const int64_t kTopSample = mController.CalculateTopOfWindow(kAvailableSampleCount);
    return mController.GetRow(aChannel, kTopSample);
}

QPolygonF
SpectrumPlot::ComputePoints(const std::vector<float>& aDecibels,
                            const size_t aWidth,
                            const size_t aHeight) const
{
    QPolygonF points;
    points.reserve(static_cast<qsizetype>(aDecibels.size()));

    const float kApertureMinDecibels = mController.GetSettings().GetApertureMinDecibels();
    const float kApertureMaxDecibels = mController.GetSettings().GetApertureMaxDecibels();
    const float kDecibelRange = kApertureMaxDecibels - kApertureMinDecibels;
    const float kImplausiblySmallDecibelRange = 1e-6f;
    if (std::abs(kDecibelRange) < kImplausiblySmallDecibelRange) {
        // We can't draw anything if the range is zero.
        return points;
    }
    const float kInverseDecibelRange = 1.0f / kDecibelRange;

    const size_t kMaxX = std::min(aWidth, aDecibels.size());
    for (size_t x = 0; x < kMaxX; x++) { // NOLINT(readability-identifier-length)
        const float kNormalizedDecibels =
          (aDecibels[x] - kApertureMinDecibels) * kInverseDecibelRange;
        const float kYCoordinate =
          static_cast<float>(aHeight) - (kNormalizedDecibels * static_cast<float>(aHeight));
        points.emplace_back(static_cast<float>(x), kYCoordinate);
    }
    return points;
}

void
SpectrumPlot::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), Qt::black);

    const size_t kChannels = mController.GetChannelCount();

    for (size_t ch = 0; ch < kChannels; ch++) {
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

        const std::vector<float> kDecibels = GetDecibels(ch);
        const QPolygonF points = ComputePoints(kDecibels, width(), height());
        painter.drawPolyline(points);
    }
}
