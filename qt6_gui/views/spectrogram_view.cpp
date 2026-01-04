#include "spectrogram_view.h"
#include "controllers/spectrogram_controller.h"
#include "include/global_constants.h"
#include <QAbstractScrollArea>
#include <QCursor>
#include <QEvent>
#include <QHBoxLayout>
#include <QImage>
#include <QOverload>
#include <QPaintEvent>
#include <QPainter>
#include <QRgb>
#include <QScrollBar>
#include <QWidget>
#include <Qt>
#include <algorithm>
#include <audio_types.h>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <stdexcept>
#include <vector>

SpectrogramView::SpectrogramView(const SpectrogramController& aController, QWidget* parent)
  : QAbstractScrollArea(parent)
  , mController(aController)
{
    constexpr int kMinWidth = 400;
    constexpr int kMinHeight = 300;
    setMinimumSize(kMinWidth, kMinHeight);

    // Initialize scrollbar settings.
    verticalScrollBar()->setObjectName("SpectrogramViewVerticalScrollBar");
    verticalScrollBar()->setMinimum(0);
    verticalScrollBar()->setMaximum(0);
    verticalScrollBar()->setSingleStep(1);

    // Repaint when scrollbar value changes, either due to user interaction or
    // when UpdateScrollbarRange is called.
    connect(
      verticalScrollBar(), &QScrollBar::valueChanged, this, QOverload<>::of(&QWidget::update));
}

void
SpectrogramView::UpdateScrollbarRange(FrameCount aAvailableFrames)
{
    // The scrollbar's maximum is the total available frames.
    const FrameCount kScrollMaximum = aAvailableFrames;

    const FFTSize kStride = mController.GetSettings().GetWindowStride();

    // Safety check for overflow.  This would only happen with an absurdly large
    // view height, but let's be safe.
    if (viewport()->height() > std::numeric_limits<int>::max() / kStride / 2) {
        throw std::overflow_error(
          std::format("{}: scroll page step exceeds int max", __PRETTY_FUNCTION__));
    }

    const auto kScrollPageStep = static_cast<int>(kStride) * viewport()->height();

    // Check if we're currently at the maximum (live mode)
    const bool kIsAtMaximum = (verticalScrollBar()->value() == verticalScrollBar()->maximum());

    // Update scrollbar range
    verticalScrollBar()->setMaximum(kScrollMaximum.ToIntChecked());
    verticalScrollBar()->setPageStep(kScrollPageStep);
    // If we were at the maximum, stay at the new maximum (follow live audio)
    if (kIsAtMaximum) {
        verticalScrollBar()->setValue(kScrollMaximum.ToIntChecked());
    }
    // Otherwise, preserve the current scroll position (user is viewing history)
}

void
SpectrogramView::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(viewport());

    auto image = GenerateSpectrogramImage(viewport()->width(), viewport()->height());

    // blit the result
    painter.drawImage(0, 0, image);

    // Overlay crosshair.  We don't need to provide any labels here, just a
    // vertical line.  The measurements happen in the SpectrumPlot view.
    const auto kMousePos = mapFromGlobal(QCursor::pos());
    const float kCrosshairPenWidth = 0.5f;
    painter.setPen(QPen(Qt::yellow, kCrosshairPenWidth, Qt::DashLine));
    painter.drawLine(kMousePos.x(), 0, kMousePos.x(), viewport()->height());
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
    const ChannelCount kChannels = mController.GetChannelCount();
    const FFTSize kStride = kSettings.GetWindowStride();

    // Determine the top frame to start rendering from.
    // The scrollbar value represents the frame position at the bottom of the view.
    // Calculate top frame by going back (height * stride) frames from the bottom.
    const FrameOffset kBottomFrame = verticalScrollBar()->value();
    const FrameOffset kTopFrameUnaligned =
      kBottomFrame - (kStride * static_cast<FrameOffset>(aHeight));
    const FrameOffset kTopFrameAligned = mController.CalculateTopOfWindow(kTopFrameUnaligned);
    const FrameOffset kTopFrame = kTopFrameAligned < 0 ? 0 : kTopFrameAligned;

    // Validate channel count.  This should never happen because AudioBuffer
    // enforces channel count limits, but let's be safe.  This guards against
    // out-of-bounds access into the color map LUTs array.
    if (kChannels > GKMaxChannels || kChannels < 1) {
        throw std::runtime_error(std::format("{}: channel count {} out of range [1, {}]",
                                             __PRETTY_FUNCTION__,
                                             kChannels,
                                             GKMaxChannels));
    }

    return RenderConfig{ .channels = kChannels,
                         .stride = kStride,
                         .top_frame = kTopFrame,
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
        decibelsChannelRowBin[ch] = mController.GetRows(ch, renderConfig.top_frame, aHeight);
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
                // and kChannels is asserted above to be <= GKMaxChannels.
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