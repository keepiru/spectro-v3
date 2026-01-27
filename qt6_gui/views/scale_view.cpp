// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "scale_view.h"
#include "controllers/spectrogram_controller.h"
#include <QColor>
#include <QFont>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QWidget>
#include <Qt>
#include <cstddef>
#include <vector>

ScaleView::ScaleView(const SpectrogramController& aController, QWidget* parent)
  : QWidget(parent)
  , mController(aController)
{
    constexpr int kScaleHeight = 20;
    setFixedHeight(kScaleHeight);
}

void
ScaleView::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    const QColor kBackgroundColor(25, 25, 50); // Dark blueish background
    painter.fillRect(rect(), kBackgroundColor);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Qt::white, 1));
    constexpr int kFontSizePoints = 8;
    painter.setFont(QFont("Sans", kFontSizePoints));
    const QFontMetrics kMetrics(painter.font());
    constexpr int kShortTickHeight = 3;

    const auto tickMarks = CalculateTickMarks(static_cast<size_t>(width()));

    // Draw tick marks and labels
    for (const auto& tick : tickMarks) {
        const int kXPos = static_cast<int>(tick.position); // Horizontal position in pixels
        if (tick.label.has_value()) {
            // Draw long tick from top to bottom
            painter.drawLine(kXPos, 0, kXPos, height());

            // Draw label
            const QString kLabel = QString::number(tick.label.value());
            const int kTextWidth = kMetrics.horizontalAdvance(kLabel);
            constexpr int kLabelOffsetY = 14;          // Hand picked offset for vertical position
            const int kLabelOffsetX = -kTextWidth - 2; // Position label slightly left of tick.
            painter.drawText(kXPos + kLabelOffsetX, kLabelOffsetY, kLabel);
        } else {
            // Draw short ticks.  One goes down, other goes up.
            painter.drawLine(kXPos, 0, kXPos, kShortTickHeight);
            painter.drawLine(kXPos, height() - kShortTickHeight, kXPos, height());
        }
    }
}

std::vector<ScaleView::TickMark>
ScaleView::CalculateTickMarks(size_t aWidth) const
{
    std::vector<TickMark> tickMarks;

    const auto kHzPerBin = mController.GetHzPerBin();
    constexpr float kDefaultHzPerTick = 200.0f;
    float hzPerTick = kDefaultHzPerTick;

    // Determine tick interval based on FFT size.  These values are chosen to
    // provide reasonable spacing of tick marks and leave room for labels.
    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    switch (mController.GetSettings().GetFFTSize()) {
        case 512:
            hzPerTick = 800;
            break;
        case 1024:
            hzPerTick = 400;
            break;
        case 2048:
            hzPerTick = 200;
            break;
        case 4096:
            hzPerTick = 100;
            break;
        case 8192:
            hzPerTick = 50;
            break;
        default:
            hzPerTick = kDefaultHzPerTick;
            break;
    }
    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

    constexpr int kTicksPerLabel = 5;
    const auto kMaxFrequency = static_cast<float>(aWidth) * kHzPerBin;
    const auto kMaxTick = static_cast<size_t>(kMaxFrequency / hzPerTick) + 1;

    for (size_t tick = 1; tick < kMaxTick; ++tick) {
        const float kHz = static_cast<float>(tick) * hzPerTick;
        const auto xPos = static_cast<size_t>(kHz / kHzPerBin);
        if (tick % kTicksPerLabel == 0) {
            // Large tick with label
            tickMarks.push_back({ .position = xPos, .label = static_cast<size_t>(kHz) });
        } else {
            // Small tick without label
            tickMarks.push_back({ .position = xPos, .label = {} });
        }
    }

    return tickMarks;
}