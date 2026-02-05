// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "spectrogram_view.h"
#include "controllers/spectrogram_controller.h"
#include "include/global_constants.h"
#include "models/colormap.h"
#include "models/settings.h"
#include <QAbstractScrollArea>
#include <QCursor>
#include <QEvent>
#include <QHBoxLayout>
#include <QImage>
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
  , mUpdateViewport([this]() { viewport()->update(); })
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
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, qOverload<>(&QWidget::update));
}

void
SpectrogramView::UpdateScrollbarRange(FrameCount aAvailableFrames)
{
    const FFTSize kStride = mController.GetSettings().GetWindowStride();
    const bool kIsLiveMode = mController.GetSettings().IsLiveMode();

    // Safety check for overflow.  This would only happen with an absurdly large
    // view height, but let's be safe.
    if (viewport()->height() > std::numeric_limits<int>::max() / kStride.AsInt()) {
        throw std::overflow_error("scroll page step exceeds int max");
    }

    const FrameCount kScrollSingleStep(kStride.Get() * 10); // 10 rows per step
    const FrameCount kScrollPageStep(kStride.Get() * viewport()->height());

    // The scrollbar's maximum is one page past the available frames, to allow
    // scrolling the end of data all the way to the top of the view.
    const FrameCount kScrollMaximum = aAvailableFrames + kScrollPageStep;

    // Update scrollbar range
    verticalScrollBar()->setMaximum(kScrollMaximum.ToIntChecked());
    verticalScrollBar()->setSingleStep(kScrollSingleStep.ToIntChecked());
    verticalScrollBar()->setPageStep(kScrollPageStep.ToIntChecked());

    // If we are in live mode, put the end of the data at the bottom of the view.
    if (kIsLiveMode) {
        verticalScrollBar()->setValue(aAvailableFrames.ToIntChecked());
    } else {
        // Otherwise, preserve the current scroll position (user is viewing
        // history).  However, we may have new data which needs to be displayed.
        const FramePosition kCurrentBottomFrame{ verticalScrollBar()->value() };
        const FramePosition kCurrentTopFrame =
          kCurrentBottomFrame - FrameCount{ kStride.Get() * viewport()->height() };

        // Check if the mPreviousAvailableFrames is inside the current view, and if
        // so, trigger a repaint - we added data that should be visible.
        if (mPreviousAvailableFrames.AsPosition() >= kCurrentTopFrame &&
            mPreviousAvailableFrames.AsPosition() < kCurrentBottomFrame) {
            // Trigger a repaint to reflect any new data
            mUpdateViewport();
        }
    }

    mPreviousAvailableFrames = aAvailableFrames;
}

void
SpectrogramView::paintEvent(QPaintEvent* /*event*/)
{
    QImage image = GenerateSpectrogramImage(viewport()->width(), viewport()->height());

    // Overlay crosshair.  We don't need to provide any labels here, just a
    // vertical line.  The measurements happen in the SpectrumPlot view.
    QPainter painter(&image);
    const auto kMousePos = mapFromGlobal(QCursor::pos());
    const float kCrosshairPenWidth = 0.5f;
    painter.setPen(QPen(Qt::yellow, kCrosshairPenWidth, Qt::DashLine));
    painter.drawLine(kMousePos.x(), 0, kMousePos.x(), viewport()->height());

    // Blit the result to the viewport
    QPainter viewportPainter(viewport());
    viewportPainter.drawImage(0, 0, image);
}

RenderConfig
SpectrogramView::GetRenderConfig(size_t aHeight) const
{
    const auto& kSettings = mController.GetSettings();
    const float kApertureFloorDecibels = kSettings.GetApertureFloorDecibels();
    const float kApertureCeilingDecibels = kSettings.GetApertureCeilingDecibels();
    const float kApertureRangeDecibels = kApertureCeilingDecibels - kApertureFloorDecibels;
    constexpr auto kColorMapMaxIndex = static_cast<float>(ColorMap::KLUTSize - 1);
    const float kApertureRangeInverseDecibels = kColorMapMaxIndex / kApertureRangeDecibels;
    const ChannelCount kChannels = mController.GetChannelCount();
    const FFTSize kStride = kSettings.GetWindowStride();

    // Determine the top frame to start rendering from.
    // The scrollbar value represents the last visible frame + a partial stride.
    // Calculate the top frame by going back one full FFT window from that
    // point, align to stride, then back up by (height - 1) * stride.
    const FramePosition kFirstPastEnd{ verticalScrollBar()->value() + 1 };
    const FramePosition kBottomFrameUnaligned = kFirstPastEnd - kSettings.GetFFTSize();
    const FramePosition kBottomFrame = mController.RoundToStride(kBottomFrameUnaligned);
    const FramePosition kTopFrame = kBottomFrame - FrameCount{ kStride * aHeight } + kStride;

    // Validate channel count.  This should never happen because AudioBuffer
    // enforces channel count limits, but let's be safe.  This guards against
    // out-of-bounds access into the color map LUTs array.
    if (kChannels > GKMaxChannels || kChannels < 1) {
        throw std::runtime_error(
          std::format("channel count {} out of range [1, {}]", kChannels, GKMaxChannels));
    }

    return RenderConfig{ .channels = kChannels,
                         .stride = kStride,
                         .top_frame = kTopFrame,
                         .aperture_floor_decibels = kApertureFloorDecibels,
                         .aperture_ceiling_decibels = kApertureCeilingDecibels,
                         .aperture_range_decibels = kApertureRangeDecibels,
                         .aperture_range_inverse_decibels = kApertureRangeInverseDecibels };
}

QImage
SpectrogramView::GenerateSpectrogramImage(int aWidth, int aHeight)
{
    QImage image(aWidth, aHeight, QImage::Format_RGBA8888);
    image.fill(Qt::black);

    const auto renderConfig = GetRenderConfig(aHeight);
    const Settings::ColorMapLUTs& kColorMapLUTs = mController.GetSettings().GetColorMapLUTs();

    constexpr float kImplausiblySmallDecibelRange = 1e-6f;
    if (std::abs(renderConfig.aperture_range_decibels) < kImplausiblySmallDecibelRange) {
        // aperture_range_inverse_decibels is infinity.  We can't draw anything if the range is
        // zero.
        return image;
    }

    // Store the magnitudes for all channels. Channel x Row x Frequency bins
    std::vector<std::vector<std::vector<float>>> decibelsChannelRowBin(renderConfig.channels);

    for (ChannelCount ch = 0; ch < renderConfig.channels; ch++) {
        decibelsChannelRowBin[ch] = mController.GetRows(ch, renderConfig.top_frame, aHeight);
    }

    // Determine max X to render, lesser of view width or data width
    const size_t kMaxX = std::min(static_cast<size_t>(aWidth), decibelsChannelRowBin[0][0].size());

    // Render spectrogram data into image
    // This is the hot path, so avoid branches and unnecessary allocations.
    for (int y = 0; y < aHeight; y++) { // NOLINT(readability-identifier-length)
        // QImage::setPixel is slow, so we're going to access the framebuffer directly
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto* const kScanLine = reinterpret_cast<uint8_t*>(image.scanLine(y));

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
            for (ChannelCount ch = 0; ch < renderConfig.channels; ch++) {
                const float kDecibels = decibelsChannelRowBin[ch][y][x];
                // Map to 0-255
                auto colorMapIndex = (kDecibels - renderConfig.aperture_floor_decibels) *
                                     renderConfig.aperture_range_inverse_decibels;
                constexpr auto kColorMapMaxIndex = static_cast<float>(ColorMap::KLUTSize - 1);
                colorMapIndex = std::clamp(colorMapIndex, 0.0f, kColorMapMaxIndex);
                // Don't use .at() here for performance in the hot path.  ch is
                // guaranteed to be in range because it's from 0 to kChannels-1,
                // and kChannels is asserted above to be <= GKMaxChannels.
                // colorMapIndex is clamped to 0-255 above, and the static_cast
                // to uint8_t guarantees that as well.
                const ColorMap::Entry kColor =
                  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                  kColorMapLUTs[ch][static_cast<uint8_t>(colorMapIndex)];
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

void
SpectrogramView::UpdateViewport()
{
    // The scrollbar range needs to be updated if the stride has changed.
    const FrameCount kAvailableFrames = mController.GetAvailableFrameCount();
    UpdateScrollbarRange(kAvailableFrames);

    mUpdateViewport();
}