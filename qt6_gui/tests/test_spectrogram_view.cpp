// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "audio_types.h"
#include "controllers/spectrogram_controller.h"
#include "fft_window.h"
#include "models/audio_buffer.h"
#include "models/settings.h"
#include "views/spectrogram_view.h"
#include <QAbstractScrollArea>
#include <QAbstractSlider>
#include <QImage>
#include <QObject>
#include <QRgb>
#include <QScrollBar>
#include <QSignalSpy>
#include <QWidget>
#include <Qt>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cstddef>
#include <cstdint>
#include <format>
#include <mock_fft_processor.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace Catch::Matchers;

/// @brief Test fixture for SpectrogramView
class TestableSpectrogramView : public SpectrogramView
{
  public:
    using SpectrogramView::SpectrogramView;

    // Expose private methods for testing
    using SpectrogramView::GenerateSpectrogramImage;
    using SpectrogramView::GetRenderConfig;

    /// @brief Override the viewport updater for testing.
    /// @param aUpdater Lambda to call for viewport updates.
    void OverrideViewportUpdater(ViewportUpdater aUpdater)
    {
        mUpdateViewport = std::move(aUpdater);
    }
};

namespace {

/// @brief Convert a QImage to a string representation for easy comparison in tests.
/// @param image The QImage to convert.
/// @return A string representation of the image's pixel RGB values.
std::string
QImageToString(const QImage& image)
{
    std::string result = "\n";
    for (int yCoord = 0; yCoord < image.height(); ++yCoord) {
        for (int xCoord = 0; xCoord < image.width(); ++xCoord) {
            auto pixel = image.pixel(xCoord, yCoord);
            result += std::format("{:02X}{:02X}{:02X} ", qRed(pixel), qGreen(pixel), qBlue(pixel));
        }
        result += '\n';
    }
    return result;
}
} // namespace

TEST_CASE("SpectrogramView constructor", "[spectrogram_view]")
{
    const Settings settings;
    const AudioBuffer audioBuffer;
    const SpectrogramController controller(settings, audioBuffer);
    const SpectrogramView view(controller);

    REQUIRE(view.minimumWidth() > 0);
    REQUIRE(view.minimumHeight() > 0);
}

TEST_CASE("SpectrogramView is widget", "[spectrogram_view]")
{
    const Settings settings;
    const AudioBuffer audioBuffer;
    const SpectrogramController controller(settings, audioBuffer);
    SpectrogramView view(controller);
    REQUIRE(qobject_cast<QWidget*>(&view) != nullptr);
}

TEST_CASE("SpectrogramView::GenerateSpectrogramImage", "[spectrogram_view]")
{
    Settings settings;

    AudioBuffer audioBuffer;
    const SpectrogramController controller(
      settings, audioBuffer, MockFFTProcessor::GetFactory(), nullptr);
    TestableSpectrogramView view(controller);

    SECTION("generates image of correct size")
    {
        constexpr size_t width = 512;
        constexpr size_t height = 256;
        const QImage image = view.GenerateSpectrogramImage(width, height);
        CHECK(image.width() == width);
        CHECK(image.height() == height);
    }

    SECTION("generates a black image if aperture range is zero")
    {
        settings.SetApertureFloorDecibels(10.0f);
        settings.SetApertureCeilingDecibels(10.0f); // zero range

        const QImage image = view.GenerateSpectrogramImage(256, 256);

        CHECK(image.width() == 256);
        CHECK(image.height() == 256);

        // Check that all pixels are black
        for (int yCoord = 0; yCoord < image.height(); ++yCoord) {
            for (int xCoord = 0; xCoord < image.width(); ++xCoord) {
                REQUIRE(image.pixel(xCoord, yCoord) == qRgb(0, 0, 0));
            }
        }
    }

    SECTION("generates correct image data based on audio buffer samples")
    {
        // Integration test pulling data through the full pipeline.

        // With this aperture and a linear colormap, the input samples 0..255 will
        // go through the mock FFT, become decibel ranges 0..255, and map directly
        // to RGB values.
        settings.SetApertureFloorDecibels(0);
        settings.SetApertureCeilingDecibels(255);

        // Fill audio buffer with known samples
        audioBuffer.Reset(1, 44100);
        audioBuffer.AddSamples({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 });

        settings.SetFFTSettings(8, FFTWindow::Type::Rectangular);
        settings.SetWindowScale(1); // no overlap

        // Update scrollbar range so frame 0 is at the top of the view
        view.UpdateScrollbarRange(FrameCount(32));

        const std::string kHave = QImageToString(view.GenerateSpectrogramImage(6, 4));
        const std::string kWant = "\n"
                                  "010001 020002 030003 040004 050005 000000 \n"
                                  "090009 0A000A 0B000B 0C000C 0D000D 000000 \n"
                                  "000000 000000 000000 000000 000000 000000 \n"
                                  "000000 000000 000000 000000 000000 000000 \n";

        REQUIRE(kHave == kWant);
    }
}

TEST_CASE("Benchmark", "[spectrogram_view][!benchmark]")
{
    Settings settings;
    const AudioBuffer audioBuffer; // No data loaded, so GetRows will return all zeros
    SpectrogramController controller(settings, audioBuffer);
    TestableSpectrogramView view(controller);

    SECTION("GenerateSpectrogramImage performance")
    {
        // Simple performance benchmark to ensure image generation is efficient.
        settings.SetFFTSettings(2048, FFTWindow::Type::Rectangular);

        // No data is loaded in the audio buffer, so GetRows will return zeroed
        // data.  Let's verify that assumption first.
        SECTION("Verify zeroed data assumption")
        {
            const auto have = controller.GetRows(0, FramePosition{ 0 }, 10);
            REQUIRE(have.size() == 10);
            for (const auto& row : have) {
                REQUIRE(row.size() == (settings.GetFFTSize() / 2) + 1);
                for (const auto& value : row) {
                    REQUIRE(value == 0.0f);
                }
            }
        }

        // Let's also see how fast GetRows is because it will affect the overall
        // image generation time.
        BENCHMARK("SpectrogramController::GetRows 1024 rows")
        {
            return controller.GetRows(0, FramePosition{ 0 }, 1024);
        };

        BENCHMARK("GenerateSpectrogramImage 800x600")
        {
            return view.GenerateSpectrogramImage(800, 600);
        };

        BENCHMARK("GenerateSpectrogramImage 1920x1080")
        {
            return view.GenerateSpectrogramImage(1920, 1080);
        };
    }
}

TEST_CASE("SpectrogramView::GetRenderConfig", "[spectrogram_view]")
{
    Settings settings;
    const AudioBuffer audioBuffer;
    const SpectrogramController controller(settings, audioBuffer);
    TestableSpectrogramView view(controller);

    SECTION("returns correct RenderConfig values")
    {

        settings.SetApertureFloorDecibels(-100.0f);
        settings.SetApertureCeilingDecibels(0.0f);
        settings.SetFFTSettings(2048, FFTWindow::Type::Hann);
        settings.SetWindowScale(2); // stride = 1024

        constexpr size_t height = 256;
        const size_t kPageStep = height * settings.GetWindowStride();

        RenderConfig want{
            .channels = 2,
            .stride = 1024,
            .top_frame = FramePosition{ 0 },
            .aperture_floor_decibels = -100.0f,
            .aperture_ceiling_decibels = 0.0f,
            .aperture_range_decibels = 100.0f,
            .aperture_range_inverse_decibels = 2.55f,
            .color_map_lut = settings.GetColorMapLUTs(),
        };

        SECTION("with zero data")
        {
            // Zero data, bottom is at frame 0, top is back one pagestep
            want.top_frame = FramePosition{ static_cast<long>(-kPageStep) };
            const RenderConfig have = view.GetRenderConfig(height);
            REQUIRE(ToString(have) == ToString(want));
        }

        SECTION("with one page of data")
        {
            view.UpdateScrollbarRange(FrameCount(kPageStep));

            want.top_frame = FramePosition{ 0 };
            const RenderConfig have = view.GetRenderConfig(height);
            REQUIRE(ToString(have) == ToString(want));
        }

        SECTION("with a page and a half of data")
        {
            const FrameCount kTotalFrames{ 0x60000 };
            view.UpdateScrollbarRange(kTotalFrames);

            want.top_frame = FramePosition{ 0x20000 };
            const RenderConfig have = view.GetRenderConfig(height);
            REQUIRE(ToString(have) == ToString(want));
        }

        SECTION("with a page and a half of data plus a bit more")
        {
            const FrameCount kTotalFrames{ 0x60000 + 1023 }; // just shy of one stride
            view.UpdateScrollbarRange(kTotalFrames);

            want.top_frame = FramePosition{ 0x20000 };
            const RenderConfig have = view.GetRenderConfig(height);
            REQUIRE(ToString(have) == ToString(want));
        }
    }
}

TEST_CASE("SpectrogramView scrollbar integration", "[spectrogram_view]")
{
    Settings settings;
    const AudioBuffer audioBuffer;
    const SpectrogramController controller(settings, audioBuffer);
    TestableSpectrogramView view(controller);

    // Set up FFT settings with known stride
    settings.SetFFTSettings(2048, FFTWindow::Type::Hann);
    settings.SetWindowScale(2); // stride = 1024

    auto* scrollBar = view.findChild<QScrollBar*>("SpectrogramViewVerticalScrollBar");
    REQUIRE(scrollBar != nullptr);

    // MainWindow connects scrollbar to Settings::ClearLiveMode, simulate that here
    QObject::connect(scrollBar, &QScrollBar::actionTriggered, &settings, &Settings::ClearLiveMode);

    // Compute the page step in frames.  This offset is relevant in scrollbar tests
    // where we need to account for the page step in the maximum value.
    const int kPageStepFrames = view.viewport()->height() * settings.GetWindowStride().AsInt();

    SECTION("scrollbar is initialized on construction")
    {
        CHECK(scrollBar->orientation() == Qt::Vertical);
        CHECK(scrollBar->minimum() == 0);
        CHECK(scrollBar->maximum() == 0);
        CHECK(scrollBar->value() == 0);
    }

    SECTION("throws when pagestep would exceed int max")
    {
        // Stride is 1024.
        // Set viewport height to a large but valid value.
        constexpr size_t kMaxAllowedHeight = (1 << 21) - 1;
        view.viewport()->setFixedHeight(kMaxAllowedHeight);

        // This makes page step:
        constexpr int kExpectedPageStep = (1LL << 31) - 1024;

        REQUIRE_NOTHROW(view.UpdateScrollbarRange(FrameCount{ 1 }));
        CHECK(scrollBar->pageStep() == kExpectedPageStep);

        // Now increase height by 1 to cause overflow.
        view.viewport()->setFixedHeight(kMaxAllowedHeight + 1);
        REQUIRE_THROWS_MATCHES(
          view.UpdateScrollbarRange(FrameCount{ 1 }),
          std::overflow_error,
          MessageMatches(ContainsSubstring("scroll page step exceeds int max")));
    }

    SECTION("throws when overflowing scrollbar maximum")
    {
        const int64_t kMaxAllowed = (1LL << 31) - kPageStepFrames - 1;
        REQUIRE_NOTHROW(view.UpdateScrollbarRange(FrameCount(kMaxAllowed)));

        const int64_t kTooLarge = kMaxAllowed + 1;
        REQUIRE_THROWS_MATCHES(
          view.UpdateScrollbarRange(FrameCount(kTooLarge)),
          std::overflow_error,
          MessageMatches(ContainsSubstring("Count(2147483648) exceeds int max")));
    }

    SECTION("UpdateScrollbarRange updates scrollbar maximum")
    {
        view.UpdateScrollbarRange(FrameCount(10000));
        REQUIRE(scrollBar->maximum() == 10000 + kPageStepFrames);
    }

    SECTION("UpdateScrollbarRange follows live data when in live mode")
    {
        settings.SetLiveMode(true);
        // Set initial range and position at max (live mode)
        view.UpdateScrollbarRange(FrameCount(10000));
        scrollBar->setValue(scrollBar->maximum());

        // Add more data
        view.UpdateScrollbarRange(FrameCount(20000));

        // Position should follow to new maximum (live mode)
        REQUIRE(scrollBar->value() == 20000);
        REQUIRE(scrollBar->maximum() == 20000 + kPageStepFrames);

        // Clear live mode
        settings.SetLiveMode(false);

        // Add more data again
        view.UpdateScrollbarRange(FrameCount(30000));

        // Position should be preserved (not live mode)
        REQUIRE(scrollBar->value() == 20000);
        REQUIRE(scrollBar->maximum() == 30000 + kPageStepFrames);
    }

    SECTION("UpdateScrollbarRange emits valueChanged signal to trigger repaint")
    {
        // We can't directly test paintEvent, but we can at least verify that
        // the scrollbar's valueChanged signal is emitted.
        QSignalSpy spy(scrollBar, &QScrollBar::valueChanged);
        view.UpdateScrollbarRange(FrameCount(10000));
        REQUIRE(spy.count() == 1);
        REQUIRE(spy.takeFirst().takeFirst().toInt() == 10000);
    }

    SECTION("Scrollbar action clears live mode in settings")
    {
        settings.SetLiveMode(true);
        REQUIRE(settings.IsLiveMode() == true);

        scrollBar->triggerAction(QAbstractSlider::SliderSingleStepAdd);

        // Live mode should be cleared
        REQUIRE(settings.IsLiveMode() == false);
    }

    SECTION("UpdateScrollbarRange repaints view if current view includes new data")
    {
        view.viewport()->setFixedHeight(256);

        // Create a test spy to count viewport updates
        size_t viewportUpdateCount = 0;
        view.OverrideViewportUpdater([&viewportUpdateCount]() noexcept { viewportUpdateCount++; });

        // Height is 256, stride is 1024, so view shows 262144 frames.
        constexpr int kPageStepFrames = 1024 * 256;

        // Set up initial data: almost one full page.
        view.UpdateScrollbarRange(FrameCount(kPageStepFrames - 2048));

        // Check our assumptions about PageStep
        REQUIRE(scrollBar->pageStep() == kPageStepFrames);

        // Clear live mode so we can test if updates happen in non-live mode.
        // This has to happen after the first UpdateScrollbarRange call to set up
        // the scrollbar.
        settings.SetLiveMode(false);
        scrollBar->setValue(kPageStepFrames); // Frame 0 at top of view

        CHECK(viewportUpdateCount == 0);

        // Add new data that falls within current view
        view.UpdateScrollbarRange(FrameCount(kPageStepFrames - 1024));
        CHECK(viewportUpdateCount == 1); // update should have been called

        // Add more data just shy of the edge of current view
        view.UpdateScrollbarRange(FrameCount(kPageStepFrames - 1));
        CHECK(viewportUpdateCount == 2); // another update should have been called

        // Add more data right at the edge of current view
        view.UpdateScrollbarRange(FrameCount(kPageStepFrames));
        CHECK(viewportUpdateCount == 3); // another update should have been called

        // Now add data that is outside current view
        view.UpdateScrollbarRange(FrameCount(kPageStepFrames + 2048));
        CHECK(viewportUpdateCount == 3); // no update should have been called

        // Go back, set the scrollbar back within the view
        view.UpdateScrollbarRange(FrameCount(kPageStepFrames - 1024));
        // no update because previous position was outside view.  The code
        // doesn't expect backtracking so it's undefined whether it should
        // actually repaint.  Currently it does not.
        CHECK(viewportUpdateCount == 3);

        // Now add data which will go past the end of the view.
        view.UpdateScrollbarRange(FrameCount(kPageStepFrames + 4096));
        CHECK(viewportUpdateCount == 4); // another update should have been called
    }

    SECTION("Scrollbar single step and page step are set correctly")
    {
        // Starting with stride = 1024
        view.UpdateScrollbarRange(FrameCount(10000));
        CHECK(scrollBar->singleStep() == 10240);
        CHECK(scrollBar->pageStep() == 1024 * view.viewport()->height());

        // Change FFT settings to change stride
        settings.SetWindowScale(4); // stride = 512
        view.UpdateScrollbarRange(FrameCount(10000));
        CHECK(scrollBar->singleStep() == 5120);
        CHECK(scrollBar->pageStep() == (512 * view.viewport()->height()));
    }
}