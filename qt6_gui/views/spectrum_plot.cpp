#include "spectrum_plot.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

#include <QPaintEvent>
#include <QPainter>
#include <QPolygonF>
#include <QWidget>
#include <Qt>

SpectrumPlot::SpectrumPlot(SpectrogramController* aController, QWidget* parent)
  : QWidget(parent)
  , mController(aController)
{
    if (mController == nullptr) {
        throw std::invalid_argument("SpectrumPlot: aController must not be null");
    }
    constexpr int kMinWidth = 400;
    constexpr int kMinHeight = 200;
    setMinimumSize(kMinWidth, kMinHeight);
}

void
SpectrumPlot::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), Qt::black);

    const auto kAvailableSampleCount = mController->GetAvailableSampleCount();
    const auto kStride = mController->GetWindowStride();
    // Round down to nearest stride
    const auto kTopSample = (kAvailableSampleCount / kStride) * kStride;
    const auto kChannels = mController->GetChannelCount();

    for (size_t ch = 0; ch < kChannels; ch++) {
        const auto kRows = mController->GetRows(ch, kTopSample, 1);
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
        points.reserve(kMagnitudes.size());

        const size_t kMaxX = std::min(kWidth, kMagnitudes.size());
        for (size_t x = 0; x < kMaxX; x++) { // NOLINT(readability-identifier-length)
            // Scale magnitude to fit in widget height
            const float kYCoordinate = kHeight - (kMagnitudes[x] * static_cast<float>(kHeight));
            points.emplace_back(static_cast<float>(x), kYCoordinate);
        }

        painter.drawPolyline(points);
    }
}
