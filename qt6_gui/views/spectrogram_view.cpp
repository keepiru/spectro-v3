#include "spectrogram_view.h"

#include "controllers/spectrogram_controller.h"

#include <QPaintEvent>
#include <QPainter>
#include <cmath>
#include <stdexcept>

SpectrogramView::SpectrogramView(SpectrogramController* aController, QWidget* parent)
  : QWidget(parent)
  , mController(aController)
{
    if (!aController) {
        throw std::invalid_argument("SpectrogramView: aController must not be null");
    }
    setMinimumSize(400, 300);
}

void
SpectrogramView::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    const size_t kChannels = mController->GetChannelCount();
    const size_t kStride = mController->GetWindowStride();
    const size_t kAvailableSampleCount = mController->GetAvailableSampleCount();
    const size_t kHeight = height();
    const size_t kWidth = width();

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
        magnitudesChannelRowBin[ch] = mController->GetRows(ch, kTopSample, kHeight);
    }

    // TODO: maybe only allocate this once and reuse
    QImage image(kWidth, kHeight, QImage::Format_RGBA8888);
    // Fill with black
    image.fill(Qt::black);

    // Determine max X to render, lesser of view width or data width
    const size_t kMaxX = std::min(kWidth, magnitudesChannelRowBin[0][0].size());

    // Render spectrogram data into image
    // This is the hot path, so avoid branches and unnecessary allocations.
    for (size_t y = 0; y < kHeight; y++) {
        // QImage::setPixel is slow, so we're going to access the framebuffer directly
        const auto kScanLine = reinterpret_cast<uint32_t*>(image.scanLine(y));

        // Scan the line.
        // This is the inner loop of the hot path, performance matters here.
        for (size_t x = 0; x < kMaxX; x++) {
            // Average magnitude across channels
            float magnitude = 0.0f;
            for (size_t ch = 0; ch < kChannels; ch++) {
                magnitude += magnitudesChannelRowBin[ch][y][x];
            }
            magnitude /= static_cast<float>(kChannels);

            // Map magnitude to color (simple grayscale for now)
            const uint8_t intensity = std::clamp(static_cast<int>(magnitude * 255.0f), 0, 255);

            kScanLine[x] = qRgba(intensity, intensity, intensity, 255);
        }
    }

    // blit the result
    painter.drawImage(0, 0, image);
}
