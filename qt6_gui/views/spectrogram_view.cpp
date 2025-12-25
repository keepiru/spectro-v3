#include "spectrogram_view.h"
#include "controllers/spectrogram_controller.h"
#include "include/global_constants.h"
#include <QImage>
#include <QPaintEvent>
#include <QPainter>
#include <QRgb>
#include <QWidget>
#include <Qt>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <stdexcept>
#include <vector>

SpectrogramView::SpectrogramView(SpectrogramController& aController, QWidget* parent)
  : QWidget(parent)
  , mController(aController)
{
    constexpr int kMinWidth = 400;
    constexpr int kMinHeight = 300;
    setMinimumSize(kMinWidth, kMinHeight);
}

void
SpectrogramView::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);

    auto image = GenerateSpectrogramImage(width(), height());

    // blit the result
    painter.drawImage(0, 0, image);
}

RenderConfig
SpectrogramView::GetRenderConfig(size_t aHeight) const
{
    const auto& kSettings = mController.GetSettings();
    const float kMinDecibels = kSettings.GetApertureMinDecibels();
    const float kMaxDecibels = kSettings.GetApertureMaxDecibels();
    const float kDecibelRange = kMaxDecibels - kMinDecibels;
    constexpr auto kColorMapMaxIndex = static_cast<float>(Settings::KColorMapLUTSize - 1);
    const float kInverseDecibelRange = kColorMapMaxIndex / kDecibelRange;
    const size_t kChannels = mController.GetChannelCount();
    const int64_t kStride = kSettings.GetWindowStride();
    const int64_t kAvailableSampleCount = mController.GetAvailableSampleCount();

    // Determine the top sample to start rendering from.
    // Go back kStride strides, then round down to nearest stride.
    // Default to 0 if the window is larger than available samples.
    const int64_t kTopSampleUnaligned =
      kAvailableSampleCount - (kStride * static_cast<int64_t>(aHeight));
    const int64_t kTopSampleAligned = mController.CalculateTopOfWindow(kTopSampleUnaligned);
    const int64_t kTopSample = kTopSampleAligned < 0 ? 0 : kTopSampleAligned;

    // Validate channel count.  This should never happen because AudioBuffer
    // enforces channel count limits, but let's be safe.  This guards against
    // out-of-bounds access into the color map LUTs array.
    if (kChannels > gkMaxChannels || kChannels < 1) {
        throw std::runtime_error(std::format("{}: channel count {} out of range [1, {}]",
                                             __PRETTY_FUNCTION__,
                                             kChannels,
                                             gkMaxChannels));
    }

    return RenderConfig{ .channels = kChannels,
                         .stride = kStride,
                         .available_sample_count = kAvailableSampleCount,
                         .top_sample = kTopSample,
                         .min_decibels = kMinDecibels,
                         .max_decibels = kMaxDecibels,
                         .decibel_range = kDecibelRange,
                         .inverse_decibel_range = kInverseDecibelRange,
                         .color_map_lut = kSettings.GetColorMapLUTs() };
}

QImage
SpectrogramView::GenerateSpectrogramImage(size_t aWidth, size_t aHeight)
{
    QImage image(static_cast<int>(aWidth), static_cast<int>(aHeight), QImage::Format_RGBA8888);
    image.fill(Qt::black);

    const auto renderConfig = GetRenderConfig(aHeight);

    constexpr float kImplausiblySmallDecibelRange = 1e-6f;
    if (std::abs(renderConfig.decibel_range) < kImplausiblySmallDecibelRange) {
        // inverse_decibel_range is infinity.  We can't draw anything if the range is zero.
        return image;
    }

    // Store the magnitudes for all channels. Channel x Row x Frequency bins
    std::vector<std::vector<std::vector<float>>> decibelsChannelRowBin(renderConfig.channels);

    for (size_t ch = 0; ch < renderConfig.channels; ch++) {
        decibelsChannelRowBin[ch] = mController.GetRows(ch, renderConfig.top_sample, aHeight);
    }

    // Determine max X to render, lesser of view width or data width
    const size_t kMaxX = std::min(aWidth, decibelsChannelRowBin[0][0].size());

    // Render spectrogram data into image
    // This is the hot path, so avoid branches and unnecessary allocations.
    for (size_t y = 0; y < aHeight; y++) { // NOLINT(readability-identifier-length)
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
            // Sum RGB values for each channel
            for (size_t ch = 0; ch < renderConfig.channels; ch++) {
                const float kDecibels = decibelsChannelRowBin[ch][y][x];
                // Map to 0-255
                auto colorMapIndex =
                  (kDecibels - renderConfig.min_decibels) * renderConfig.inverse_decibel_range;
                constexpr auto kColorMapMaxIndex =
                  static_cast<float>(Settings::KColorMapLUTSize - 1);
                colorMapIndex = std::clamp(colorMapIndex, 0.0f, kColorMapMaxIndex);
                // Don't use .at() here for performance in the hot path.  ch is
                // guaranteed to be in range because it's from 0 to kChannels-1,
                // and kChannels is asserted above to be <= gkMaxChannels.
                // colorMapIndex is clamped to 0-255 above, and the static_cast
                // to uint8_t guarantees that as well.
                const Settings::ColorMapEntry kColor =
                  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                  renderConfig.color_map_lut[ch][static_cast<uint8_t>(colorMapIndex)];
                r += kColor.r;
                g += kColor.g;
                b += kColor.b;
            }

            // Clamp final RGB values to [0, 255]
            constexpr int kMaxColorChannelValue = std::numeric_limits<uint8_t>::max();
            r = std::min(r, kMaxColorChannelValue);
            g = std::min(g, kMaxColorChannelValue);
            b = std::min(b, kMaxColorChannelValue);

            // As above, we're accessing the framebuffer directly for performance
            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            constexpr size_t kBytesPerRGBAPixel = 4;
            const size_t kScanLineIndex = x * kBytesPerRGBAPixel;
            kScanLine[kScanLineIndex + 0] = static_cast<uint8_t>(r); // R
            kScanLine[kScanLineIndex + 1] = static_cast<uint8_t>(g); // G
            kScanLine[kScanLineIndex + 2] = static_cast<uint8_t>(b); // B
            kScanLine[kScanLineIndex + 3] = kMaxColorChannelValue;   // A
            // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
    }
    return image;
}