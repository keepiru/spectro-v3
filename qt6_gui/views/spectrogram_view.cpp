#include "spectrogram_view.h"

#include "controllers/spectrogram_controller.h"

#include <QImage>
#include <QPaintEvent>
#include <QPainter>
#include <QWidget>
#include <Qt>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <dsp_utils.h>
#include <stdexcept>
#include <vector>

SpectrogramView::SpectrogramView(SpectrogramController* aController, QWidget* parent)
  : QWidget(parent)
  , mController(aController)
  , mColorMapLUT()
{
    if (!aController) {
        throw std::invalid_argument("SpectrogramView: aController must not be null");
    }
    constexpr int kMinWidth = 400;
    constexpr int kMinHeight = 300;
    setMinimumSize(kMinWidth, kMinHeight);
    SetColorMap(ColorMapType::kGrayscale);
}

void
SpectrogramView::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);

    const size_t kChannels = mController->GetChannelCount();
    const size_t kStride = mController->GetWindowStride();
    const size_t kAvailableSampleCount = mController->GetAvailableSampleCount();
    const size_t kHeight = height();
    const size_t kWidth = width();
    const float kMinDecibels = mController->GetApertureMinDecibels();
    const float kMaxDecibels = mController->GetApertureMaxDecibels();
    const float kInverseDecibelRange = 255.0f / (kMaxDecibels - kMinDecibels);

    // Determine the top sample to start rendering from.
    // Go back kStride strides, then round down to nearest stride.
    // Default to 0 if the window is larger than available samples.
    const size_t kTopSample =
      (kStride * kHeight < kAvailableSampleCount)
        ? ((kAvailableSampleCount - (kStride * kHeight)) / kStride) * kStride
        : 0;

    // Store the magnitudes for all channels. Channel x Row x Frequency bins
    std::vector<std::vector<std::vector<float>>> magnitudesChannelRowBin(kChannels);

    for (size_t ch = 0; ch < kChannels; ch++) {
        magnitudesChannelRowBin[ch] =
          mController->GetRows(ch, static_cast<int64_t>(kTopSample), kHeight);
    }

    // TODO: maybe only allocate this once and reuse
    QImage image(static_cast<int>(kWidth), static_cast<int>(kHeight), QImage::Format_RGBA8888);
    // Fill with black
    image.fill(Qt::black);

    // Determine max X to render, lesser of view width or data width
    const size_t kMaxX = std::min(kWidth, magnitudesChannelRowBin[0][0].size());

    // Render spectrogram data into image
    // This is the hot path, so avoid branches and unnecessary allocations.
    for (size_t y = 0; y < kHeight; y++) { // NOLINT(readability-identifier-length)
        // QImage::setPixel is slow, so we're going to access the framebuffer directly
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto* const kScanLine = reinterpret_cast<uint8_t*>(image.scanLine(static_cast<int>(y)));

        // Scan the line.
        // This is the inner loop of the hot path, performance matters here.
        for (size_t x = 0; x < kMaxX; x++) { // NOLINT(readability-identifier-length)
            // composite the colors from each channel in decibels
            // NOLINTBEGIN(readability-identifier-length)
            int r = 0;
            int g = 0;
            int b = 0;
            // NOLINTEND(readability-identifier-length)
            // First sum RGB values for each channel
            for (size_t ch = 0; ch < kChannels; ch++) {
                const float kMagnitude = magnitudesChannelRowBin[ch][y][x];
                const float kDecibels = DSPUtils::MagnitudeToDecibels(kMagnitude);
                // Map to 0-255
                auto colorMapIndex = (kDecibels - kMinDecibels) * kInverseDecibelRange;
                const float kMaxIndex = 255.0f;
                colorMapIndex = std::clamp(colorMapIndex, 0.0f, kMaxIndex);
                const auto kColor = mColorMapLUT.at(static_cast<size_t>(colorMapIndex));
                r += kColor.r;
                g += kColor.g;
                b += kColor.b;
            }
            // Then normalize by channel count
            r /= static_cast<int>(kChannels);
            g /= static_cast<int>(kChannels);
            b /= static_cast<int>(kChannels);

            // As above, we're accessing the framebuffer directly for performance
            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            const size_t kScanLineIndex = x * 4;
            const uint8_t kAlpha = 255;
            kScanLine[kScanLineIndex + 0] = static_cast<uint8_t>(r); // R
            kScanLine[kScanLineIndex + 1] = static_cast<uint8_t>(g); // G
            kScanLine[kScanLineIndex + 2] = static_cast<uint8_t>(b); // B
            kScanLine[kScanLineIndex + 3] = kAlpha;                  // A
            // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
    }

    // blit the result
    painter.drawImage(0, 0, image);
}

void
SpectrogramView::SetColorMap(ColorMapType aType)
{
    const size_t kLUTSize = mColorMapLUT.size();
    switch (aType) {
        case ColorMapType::kGrayscale:
            for (size_t i = 0; i < kLUTSize; i++) {
                const auto intensity = static_cast<uint8_t>(i);
                mColorMapLUT.at(i) =
                  ColorMapEntry{ .r = intensity, .g = intensity, .b = intensity };
            }
            break;
        default:
            throw std::invalid_argument("SpectrogramView::SetColorMap: Unsupported color map type");
    }
}