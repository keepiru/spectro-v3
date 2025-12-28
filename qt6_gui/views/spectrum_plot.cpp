#include "spectrum_plot.h"
#include "controllers/spectrogram_controller.h"
#include <QLine>
#include <QPaintEvent>
#include <QPainter>
#include <QPolygonF>
#include <QWidget>
#include <Qt>
#include <QtTypes>
#include <algorithm>
#include <array>
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

    constexpr int kFontSizePoints = 8;
    painter.setPen(Qt::white);
    painter.setFont(QFont("Sans", kFontSizePoints));

    const DecibelScaleParameters params = CalculateDecibelScaleParameters(height());
    const std::vector<DecibelMarker> markers = GenerateDecibelScaleMarkers(params, width());
    for (const auto& marker : markers) {
        painter.drawLine(marker.line);
        painter.drawText(marker.rect, Qt::AlignRight | Qt::AlignVCenter, marker.text);
    }
}

SpectrumPlot::DecibelScaleParameters
SpectrumPlot::CalculateDecibelScaleParameters(const int aHeight) const
{
    // Compute the marker positions and spacing
    const auto& kSettings = mController.GetSettings();
    const float kApertureMinDecibels = kSettings.GetApertureMinDecibels();
    const float kApertureMaxDecibels = kSettings.GetApertureMaxDecibels();

    if (kApertureMinDecibels == kApertureMaxDecibels) {
        // Can't compute markers if the range is zero.  We'll set .marker_count
        // to 0, which will cause GenerateDecibelScaleMarkers() to return an
        // empty vector.
        return { .aperture_min_decibels = kApertureMinDecibels,
                 .aperture_max_decibels = kApertureMaxDecibels,
                 .pixels_per_decibel = 0.0f,
                 .decibel_step = 0,
                 .top_marker_decibels = 0.0f,
                 .marker_count = 0 };
    }

    // Pixels per decibel - may be negative if min > max
    const float kPixelsPerDecibel =
      static_cast<float>(aHeight) / (kApertureMaxDecibels - kApertureMinDecibels);

    // Choose the smallest step which ensures minimum spacing
    constexpr int kMinSpacingPx = 20;
    constexpr std::array<int, 6> kPossibleSteps = { 1, 2, 5, 10, 20, 50 };
    int decibelStep = kPossibleSteps.back(); // Default to largest if none work
    for (const int step : kPossibleSteps) {
        if (std::abs(static_cast<float>(step) * kPixelsPerDecibel) >= kMinSpacingPx) {
            decibelStep = step;
            break;
        }
    }

    // Step backwards if the scale is inverted.
    // Note: decibelStep is intentionally allowed to be negative here so that
    // subsequent calculations (kTopMarkerDecibels, kMarkerCount and the loop
    // stepping by i * decibelStep) naturally handle an inverted decibel axis.
    if (kPixelsPerDecibel < 0.0f) {
        decibelStep = -decibelStep;
    }

    const float kTopMarkerDecibels =
      std::ceil(kApertureMaxDecibels / static_cast<float>(decibelStep)) *
      static_cast<float>(decibelStep);
    const int kMarkerCount =
      (static_cast<int>(kApertureMaxDecibels - kApertureMinDecibels) / decibelStep) + 1;

    return { .aperture_min_decibels = kApertureMinDecibels,
             .aperture_max_decibels = kApertureMaxDecibels,
             .pixels_per_decibel = kPixelsPerDecibel,
             .decibel_step = decibelStep,
             .top_marker_decibels = kTopMarkerDecibels,
             .marker_count = kMarkerCount };
}

std::vector<SpectrumPlot::DecibelMarker>
SpectrumPlot::GenerateDecibelScaleMarkers(const DecibelScaleParameters& aParams, const int aWidth)
{
    // Tick mark settings
    constexpr int kTickMarkWidth = 10;

    // Label dimensions and offsets
    constexpr int kLabelWidth = 20;
    constexpr int kLabelHeight = 10;
    constexpr int kLabelOffsetX = kTickMarkWidth + kLabelWidth + 5; // from the right edge
    constexpr int kLabelOffsetY = kLabelHeight / 2;
    const int32_t kLabelPositionX = aWidth - kLabelOffsetX;

    if (aParams.marker_count == 0) {
        return {};
    }

    std::vector<DecibelMarker> markers;

    for (int i = 0; i < aParams.marker_count; i++) {
        // Compute the Y position for this decibel level
        const float kDecibels =
          aParams.top_marker_decibels - static_cast<float>(i * aParams.decibel_step);
        const auto kYPosition = static_cast<int32_t>((aParams.aperture_max_decibels - kDecibels) *
                                                     aParams.pixels_per_decibel);

        const QLine kTickLine(aWidth - kTickMarkWidth, kYPosition, aWidth, kYPosition);
        const QRect kLabelRect(
          kLabelPositionX, kYPosition - kLabelOffsetY, kLabelWidth, kLabelHeight);
        const QString kDecibelString = QString::number(static_cast<int>(kDecibels));
        markers.push_back(
          DecibelMarker{ .line = kTickLine, .rect = kLabelRect, .text = kDecibelString });
    }
    return markers;
}