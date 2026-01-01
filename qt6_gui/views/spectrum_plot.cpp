#include "spectrum_plot.h"
#include "controllers/spectrogram_controller.h"
#include "include/global_constants.h"
#include <QCursor>
#include <QLine>
#include <QPaintEvent>
#include <QPainter>
#include <QPolygonF>
#include <QWidget>
#include <Qt>
#include <QtTypes>
#include <algorithm>
#include <array>
#include <audio_types.h>
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
SpectrumPlot::GetDecibels(const ChannelCount aChannel) const
{
    const FrameCount kAvailableFrameCount = mController.GetAvailableFrameCount();
    // This casting friction should be fixed after we implement cursors correctly.
    const FrameOffset kTopFrame =
      mController.CalculateTopOfWindow(static_cast<FrameOffset>(kAvailableFrameCount));
    return mController.GetRow(aChannel, kTopFrame);
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

    // Draw the spectrum for each channel
    const ChannelCount kChannels = mController.GetChannelCount();

    for (ChannelCount ch = 0; ch < kChannels; ch++) {
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

    // Draw the decibel scale markers
    constexpr int kFontSizePoints = 8;
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", kFontSizePoints));

    const DecibelScaleParameters params = CalculateDecibelScaleParameters(height());
    const std::vector<Marker> markers = GenerateDecibelScaleMarkers(params, width());
    for (const auto& marker : markers) {
        painter.drawLine(marker.line);
        painter.drawText(marker.rect, Qt::AlignRight | Qt::AlignVCenter, marker.text);
    }

    // Draw crosshair at mouse position
    const auto kMousePos = mapFromGlobal(QCursor::pos());
    const auto kMarkers = ComputeCrosshair(kMousePos, height(), width());

    // dashed yellow line
    const float kCrosshairPenWidth = 0.5f;
    painter.setPen(QPen(Qt::yellow, kCrosshairPenWidth, Qt::DashLine));

    // Vertical line for frequency
    painter.drawLine(kMarkers[0].line);
    painter.drawText(kMarkers[0].rect, Qt::AlignLeft | Qt::AlignVCenter, kMarkers[0].text);

    // Horizontal line for decibel level
    painter.drawLine(kMarkers[1].line);
    painter.drawText(kMarkers[1].rect, Qt::AlignRight | Qt::AlignBottom, kMarkers[1].text);
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

std::vector<SpectrumPlot::Marker>
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

    std::vector<Marker> markers;

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
        markers.push_back(Marker{ .line = kTickLine, .rect = kLabelRect, .text = kDecibelString });
    }
    return markers;
}

std::array<SpectrumPlot::Marker, 2>
SpectrumPlot::ComputeCrosshair(QPoint aMousePos, int aHeight, int aWidth) const
{
    // Vertical line for frequency
    const QLine kFrequencyLine(aMousePos.x(), 0, aMousePos.x(), aHeight);

    // Position label on the top, to the right of the line
    const QRect kFrequencyLabelRect(aMousePos.x() + 5, 5, 50, 10);

    // Compute frequency at mouse X position
    const float kHzPerBin = mController.GetHzPerBin();
    const auto kFrequencyHz = static_cast<int32_t>(static_cast<float>(aMousePos.x()) * kHzPerBin);
    const QString kFrequencyText = QString::number(kFrequencyHz) + " Hz";

    const Marker kFrequencyMarker{ .line = kFrequencyLine,
                                   .rect = kFrequencyLabelRect,
                                   .text = kFrequencyText };

    // Horizontal line for decibel level
    const QLine kDecibelLine(0, aMousePos.y(), aWidth, aMousePos.y());

    // Position label on the right side, above the line, to the left of dB scale
    const QRect kDecibelLabelRect(aWidth - 100, aMousePos.y() - 18, 60, 15);

    // Compute decibel value at mouse Y position
    const auto& kSettings = mController.GetSettings();
    const float kApertureMinDecibels = kSettings.GetApertureMinDecibels();
    const float kApertureMaxDecibels = kSettings.GetApertureMaxDecibels();
    const float kDecibelRange = kApertureMaxDecibels - kApertureMinDecibels;
    const float kNormalizedY =
      1.0f - (static_cast<float>(aMousePos.y()) / static_cast<float>(aHeight));
    const float kDecibelValue = kApertureMinDecibels + (kNormalizedY * kDecibelRange);
    const QString kDecibelText = QString::number(static_cast<int>(kDecibelValue)) + " dB";

    const Marker kDecibelMarker{ .line = kDecibelLine,
                                 .rect = kDecibelLabelRect,
                                 .text = kDecibelText };

    return { kFrequencyMarker, kDecibelMarker };
}