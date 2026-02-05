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

/// @brief Testable subclass of SpectrogramView that exposes private members for testing
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

/// @brief Common test fixture for SpectrogramView tests
struct SpectrogramViewTestFixture
{
    Settings settings;
    AudioBuffer audio_buffer;
    SpectrogramController controller{ settings, audio_buffer, MockFFTProcessor::GetFactory() };
    TestableSpectrogramView view{ controller };
};

} // namespace

TEST_CASE("SpectrogramView constructor", "[spectrogram_view]")
{
    const SpectrogramViewTestFixture fixture;

    REQUIRE(fixture.view.minimumWidth() > 0);
    REQUIRE(fixture.view.minimumHeight() > 0);
}

TEST_CASE("SpectrogramView is widget", "[spectrogram_view]")
{
    SpectrogramViewTestFixture fixture;
    REQUIRE(qobject_cast<QWidget*>(&fixture.view) != nullptr);
}

TEST_CASE("SpectrogramView::GenerateSpectrogramImage", "[spectrogram_view]")
{
    SpectrogramViewTestFixture fixture;

    SECTION("generates image of correct size")
    {
        constexpr size_t width = 512;
        constexpr size_t height = 256;
        const QImage image = fixture.view.GenerateSpectrogramImage(width, height);
        CHECK(image.width() == width);
        CHECK(image.height() == height);
    }

    SECTION("generates a black image if aperture range is zero")
    {
        fixture.settings.SetApertureFloorDecibels(10.0f);
        fixture.settings.SetApertureCeilingDecibels(10.0f); // zero range

        const QImage image = fixture.view.GenerateSpectrogramImage(256, 256);

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
        fixture.settings.SetApertureFloorDecibels(0);
        fixture.settings.SetApertureCeilingDecibels(255);

        // Fill audio buffer with known samples
        fixture.audio_buffer.Reset(1, 44100);
        fixture.audio_buffer.AddSamples({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 });

        fixture.settings.SetFFTSettings(8, FFTWindow::Type::Rectangular);
        fixture.settings.SetWindowScale(1); // no overlap

        // Update scrollbar range so frame 0 is at the top of the view
        fixture.view.UpdateScrollbarRange(FrameCount(32));

        const std::string kHave = QImageToString(fixture.view.GenerateSpectrogramImage(6, 4));
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
    SpectrogramViewTestFixture fixture;

    SECTION("GenerateSpectrogramImage performance")
    {
        // Simple performance benchmark to ensure image generation is efficient.
        fixture.settings.SetFFTSettings(2048, FFTWindow::Type::Rectangular);
        // No data is loaded in the audio buffer, so GetRows will return zeroed
        // data.  Let's verify that assumption first.
        SECTION("Verify zeroed data assumption")
        {
            const auto have = fixture.controller.GetRows(0, FramePosition{ 0 }, 10);
            REQUIRE(have.size() == 10);
            for (const auto& row : have) {
                REQUIRE(row.size() == (fixture.settings.GetFFTSize() / 2) + 1);
                for (const auto& value : row) {
                    REQUIRE(value == 0.0f);
                }
            }
        }

        // Let's also see how fast GetRows is because it will affect the overall
        // image generation time.
        BENCHMARK("SpectrogramController::GetRows 1024 rows")
        {
            return fixture.controller.GetRows(0, FramePosition{ 0 }, 1024);
        };

        BENCHMARK("GenerateSpectrogramImage 800x600")
        {
            return fixture.view.GenerateSpectrogramImage(800, 600);
        };

        BENCHMARK("GenerateSpectrogramImage 1920x1080")
        {
            return fixture.view.GenerateSpectrogramImage(1920, 1080);
        };
    }
}

TEST_CASE("SpectrogramView::GetRenderConfig", "[spectrogram_view]")
{
    SpectrogramViewTestFixture fixture;

    SECTION("returns correct RenderConfig values")
    {

        fixture.settings.SetApertureFloorDecibels(-100.0f);
        fixture.settings.SetApertureCeilingDecibels(0.0f);
        fixture.settings.SetFFTSettings(8, FFTWindow::Type::Hann);
        constexpr size_t kHeight = 5;

        RenderConfig want{
            .channels = 2,
            .stride = 4,
            .top_frame = FramePosition{ 0 },
            .aperture_floor_decibels = -100.0f,
            .aperture_ceiling_decibels = 0.0f,
            .aperture_range_decibels = 100.0f,
            .aperture_range_inverse_decibels = 2.55f,
        };

        auto getTopFrame = [&fixture](const int aScrollbarPosition) -> int64_t {
            fixture.view.UpdateScrollbarRange(FrameCount{ 10000 });
            // Set scrollbar position
            auto* scrollBar =
              fixture.view.findChild<QScrollBar*>("SpectrogramViewVerticalScrollBar");
            REQUIRE(scrollBar != nullptr);
            scrollBar->setValue(aScrollbarPosition);

            const RenderConfig have = fixture.view.GetRenderConfig(kHeight);
            return have.top_frame.Get();
        };

        SECTION("scale=2 / stride=4")
        {
            fixture.settings.SetWindowScale(2); // stride = 4

            const FFTSize kStride = fixture.settings.GetWindowStride();
            REQUIRE(kStride == FFTSize{ 4 });
            want.stride = kStride;

            const size_t kPageStep = kHeight * kStride;
            REQUIRE(kPageStep == 20);

            SECTION("scrolled to zero")
            {
                // [-24,-23,-22,-21,-20,-19,-18,-17]
                // [-20,-19,-18,-17,-16,-15,-14,-13]
                // [-16,-15,-14,-13,-12,-11,-10, -9]
                // [-12,-11,-10, -9, -8, -7, -6, -5]
                // [ -8, -7, -6, -5, -4, -3, -2, -1]

                // We are 6 strides back.  Check our assumptions:
                REQUIRE(6 * kStride == 24);

                CHECK(getTopFrame(0) == -24); // Scrollbar at 0
                CHECK(getTopFrame(1) == -24);
                CHECK(getTopFrame(2) == -24);

                CHECK(getTopFrame(3) == -20);
                CHECK(getTopFrame(4) == -20);

                fixture.view.UpdateScrollbarRange(FrameCount{ 0 });
                want.top_frame = FramePosition{ -24 };
                const RenderConfig kHave = fixture.view.GetRenderConfig(kHeight);
                REQUIRE(ToString(kHave) == ToString(want));
            }

            SECTION("scrolled down 1 page")
            {
                // [ 0,  1,  2,  3,  4,  5,  6,  7]
                // [ 4,  5,  6,  7,  8,  9, 10, 11]
                // [ 8,  9, 10, 11, 12, 13, 14, 15]
                // [12, 13, 14, 15, 16, 17, 18, 19]
                // [16, 17, 18, 19, 20, 21, 22, 23]

                // We need kPageStep + 1 stride.  Check our assumptions:
                REQUIRE(kPageStep + kStride == 24);

                CHECK(getTopFrame(21) == -4);
                CHECK(getTopFrame(22) == -4);
                CHECK(getTopFrame(23) == 0);
                CHECK(getTopFrame(24) == 0);

                fixture.view.UpdateScrollbarRange(FrameCount{ 23 });
                want.top_frame = FramePosition{ 0 };
                const RenderConfig kHave = fixture.view.GetRenderConfig(kHeight);
                REQUIRE(ToString(kHave) == ToString(want));
            }

            SECTION("scrolled down about 1 and a half pages")
            {
                // [ 8,  9, 10, 11, 12, 13, 14, 15]
                // [12, 13, 14, 15, 16, 17, 18, 19]
                // [16, 17, 18, 19, 20, 21, 22, 23]
                // [20, 21, 22, 23, 24, 25, 26, 27]
                // [24, 25, 26, 27, 28, 29, 30, 31]

                // We need kPageStep + 3 strides.  Check our assumptions:
                REQUIRE(kPageStep + (3 * kStride) == 32);

                CHECK(getTopFrame(29) == 4);
                CHECK(getTopFrame(30) == 4);
                CHECK(getTopFrame(31) == 8);
                CHECK(getTopFrame(32) == 8);

                fixture.view.UpdateScrollbarRange(FrameCount{ 31 });
                want.top_frame = FramePosition{ 8 };
                const RenderConfig kHave = fixture.view.GetRenderConfig(kHeight);
                REQUIRE(ToString(kHave) == ToString(want));

                SECTION("window height 0")
                {
                    // We should handle this gracefully.  Maybe the user is resizing the window.
                    want.top_frame = FramePosition{ 28 };
                    const RenderConfig kHave = fixture.view.GetRenderConfig(0);
                    REQUIRE(ToString(kHave) == ToString(want));
                }
            }
        }

        SECTION("scale=1 / stride=8 (no overlap)")
        {
            fixture.settings.SetWindowScale(1); // stride = 8

            const FFTSize kStride = fixture.settings.GetWindowStride();
            REQUIRE(kStride == FFTSize{ 8 });
            want.stride = kStride;

            const size_t kPageStep = kHeight * kStride;
            REQUIRE(kPageStep == 40);

            SECTION("4 rows down")
            {
                // [-8, -7, -6, -5, -4, -3, -2, -1]
                // [ 0,  1,  2,  3,  4,  5,  6,  7]
                // [ 8,  9, 10, 11, 12, 13, 14, 15]
                // [16, 17, 18, 19, 20, 21, 22, 23]
                // [24, 25, 26, 27, 28, 29, 30, 31]

                // We need 1 page - 1 stride.  Check our assumptions:
                REQUIRE(kPageStep - kStride == 32);

                CHECK(getTopFrame(29) == -16);
                CHECK(getTopFrame(30) == -16);
                CHECK(getTopFrame(31) == -8);
                CHECK(getTopFrame(32) == -8);

                fixture.view.UpdateScrollbarRange(FrameCount{ 31 });
                want.top_frame = FramePosition{ -8 };
                const RenderConfig kHave = fixture.view.GetRenderConfig(kHeight);
                REQUIRE(ToString(kHave) == ToString(want));
            }
        }

        SECTION("scale=8 / stride=1 (one frame per stride)")
        {
            fixture.settings.SetWindowScale(8); // stride = 1

            const FFTSize kStride = fixture.settings.GetWindowStride();
            REQUIRE(kStride == FFTSize{ 1 });
            want.stride = kStride;

            const size_t kPageStep = kHeight * kStride;
            REQUIRE(kPageStep == 5);

            SECTION("scrolled 18 rows down")
            {
                // [14, 15, 16, 17, 18, 19, 20, 21]
                // [15, 16, 17, 18, 19, 20, 21, 22]
                // [16, 17, 18, 19, 20, 21, 22, 23]
                // [17, 18, 19, 20, 21, 22, 23, 24]
                // [18, 19, 20, 21, 22, 23, 24, 25]

                // We need 14 frames + 4 strides.  Check our assumptions:
                REQUIRE(14 + (4 * kStride) == 18);

                CHECK(getTopFrame(23) == 12);
                CHECK(getTopFrame(24) == 13);
                CHECK(getTopFrame(25) == 14);
                CHECK(getTopFrame(26) == 15);

                fixture.view.UpdateScrollbarRange(FrameCount{ 25 });
                want.top_frame = FramePosition{ 14 };
                const RenderConfig kHave = fixture.view.GetRenderConfig(kHeight);
                REQUIRE(ToString(kHave) == ToString(want));
            }
        }
    }
}

TEST_CASE("SpectrogramView scrollbar integration", "[spectrogram_view]")
{
    SpectrogramViewTestFixture fixture;

    // Set up FFT settings with known stride
    fixture.settings.SetFFTSettings(2048, FFTWindow::Type::Hann);
    fixture.settings.SetWindowScale(2); // stride = 1024
    auto* scrollBar = fixture.view.findChild<QScrollBar*>("SpectrogramViewVerticalScrollBar");
    REQUIRE(scrollBar != nullptr);

    // MainWindow connects scrollbar to Settings::ClearLiveMode, simulate that here
    QObject::connect(
      scrollBar, &QScrollBar::actionTriggered, &fixture.settings, &Settings::ClearLiveMode);

    // Compute the page step in frames.  This offset is relevant in scrollbar tests
    // where we need to account for the page step in the maximum value.
    const int kPageStepFrames =
      fixture.view.viewport()->height() * fixture.settings.GetWindowStride().AsInt();
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
        fixture.view.viewport()->setFixedHeight(kMaxAllowedHeight);

        // This makes page step:
        constexpr int kExpectedPageStep = (1LL << 31) - 1024;

        REQUIRE_NOTHROW(fixture.view.UpdateScrollbarRange(FrameCount{ 1 }));
        CHECK(scrollBar->pageStep() == kExpectedPageStep);

        // Now increase height by 1 to cause overflow.
        fixture.view.viewport()->setFixedHeight(kMaxAllowedHeight + 1);
        REQUIRE_THROWS_MATCHES(
          fixture.view.UpdateScrollbarRange(FrameCount{ 1 }),
          std::overflow_error,
          MessageMatches(Catch::Matchers::ContainsSubstring("scroll page step exceeds int max")));
    }

    SECTION("throws when overflowing scrollbar maximum")
    {
        const int64_t kMaxAllowed = (1LL << 31) - kPageStepFrames - 1;
        REQUIRE_NOTHROW(fixture.view.UpdateScrollbarRange(FrameCount(kMaxAllowed)));

        const int64_t kTooLarge = kMaxAllowed + 1;
        REQUIRE_THROWS_MATCHES(
          fixture.view.UpdateScrollbarRange(FrameCount(kTooLarge)),
          std::overflow_error,
          MessageMatches(Catch::Matchers::ContainsSubstring("Count(2147483648) exceeds int max")));
    }

    SECTION("UpdateScrollbarRange updates scrollbar maximum")
    {
        fixture.view.UpdateScrollbarRange(FrameCount(10000));
        REQUIRE(scrollBar->maximum() == 10000 + kPageStepFrames);
    }

    SECTION("UpdateScrollbarRange follows live data when in live mode")
    {
        fixture.settings.SetLiveMode(true);
        // Set initial range and position at max (live mode)
        fixture.view.UpdateScrollbarRange(FrameCount(10000));
        scrollBar->setValue(scrollBar->maximum());

        // Add more data
        fixture.view.UpdateScrollbarRange(FrameCount(20000));

        // Position should follow to new maximum (live mode)
        REQUIRE(scrollBar->value() == 20000);
        REQUIRE(scrollBar->maximum() == 20000 + kPageStepFrames);

        // Clear live mode
        fixture.settings.SetLiveMode(false);

        // Add more data again
        fixture.view.UpdateScrollbarRange(FrameCount(30000));

        // Position should be preserved (not live mode)
        REQUIRE(scrollBar->value() == 20000);
        REQUIRE(scrollBar->maximum() == 30000 + kPageStepFrames);
    }

    SECTION("UpdateScrollbarRange emits valueChanged signal to trigger repaint")
    {
        // We can't directly test paintEvent, but we can at least verify that
        // the scrollbar's valueChanged signal is emitted.
        QSignalSpy spy(scrollBar, &QScrollBar::valueChanged);
        fixture.view.UpdateScrollbarRange(FrameCount(10000));
        REQUIRE(spy.count() == 1);
        REQUIRE(spy.takeFirst().takeFirst().toInt() == 10000);
    }

    SECTION("Scrollbar action clears live mode in settings")
    {
        fixture.settings.SetLiveMode(true);
        REQUIRE(fixture.settings.IsLiveMode() == true);

        scrollBar->triggerAction(QAbstractSlider::SliderSingleStepAdd);

        // Live mode should be cleared
        REQUIRE(fixture.settings.IsLiveMode() == false);
    }

    SECTION("UpdateScrollbarRange repaints view if current view includes new data")
    {
        fixture.view.viewport()->setFixedHeight(256);

        // Create a test spy to count viewport updates
        size_t viewportUpdateCount = 0;
        fixture.view.OverrideViewportUpdater(
          [&viewportUpdateCount]() noexcept { viewportUpdateCount++; });

        // Height is 256, stride is 1024, so view shows 262144 frames.
        constexpr int kPageStepFrames = 1024 * 256;

        // Set up initial data: almost one full page.
        fixture.view.UpdateScrollbarRange(FrameCount(kPageStepFrames - 2048));

        // Check our assumptions about PageStep
        REQUIRE(scrollBar->pageStep() == kPageStepFrames);

        // Clear live mode so we can test if updates happen in non-live mode.
        // This has to happen after the first UpdateScrollbarRange call to set up
        // the scrollbar.
        fixture.settings.SetLiveMode(false);
        scrollBar->setValue(kPageStepFrames); // Frame 0 at top of view

        CHECK(viewportUpdateCount == 0);

        // Add new data that falls within current view
        fixture.view.UpdateScrollbarRange(FrameCount(kPageStepFrames - 1024));
        CHECK(viewportUpdateCount == 1); // update should have been called

        // Add more data just shy of the edge of current view
        fixture.view.UpdateScrollbarRange(FrameCount(kPageStepFrames - 1));
        CHECK(viewportUpdateCount == 2); // another update should have been called

        // Add more data right at the edge of current view
        fixture.view.UpdateScrollbarRange(FrameCount(kPageStepFrames));
        CHECK(viewportUpdateCount == 3); // another update should have been called

        // Now add data that is outside current view
        fixture.view.UpdateScrollbarRange(FrameCount(kPageStepFrames + 2048));
        CHECK(viewportUpdateCount == 3); // no update should have been called

        // Go back, set the scrollbar back within the view
        fixture.view.UpdateScrollbarRange(FrameCount(kPageStepFrames - 1024));
        // no update because previous position was outside view.  The code
        // doesn't expect backtracking so it's undefined whether it should
        // actually repaint.  Currently it does not.
        CHECK(viewportUpdateCount == 3);

        // Now add data which will go past the end of the view.
        fixture.view.UpdateScrollbarRange(FrameCount(kPageStepFrames + 4096));
        CHECK(viewportUpdateCount == 4); // another update should have been called
    }

    SECTION("Scrollbar single step and page step are set correctly")
    {
        // Starting with stride = 1024
        fixture.view.UpdateScrollbarRange(FrameCount(10000));
        CHECK(scrollBar->singleStep() == 10240);
        CHECK(scrollBar->pageStep() == 1024 * fixture.view.viewport()->height());

        // Change FFT settings to change stride
        fixture.settings.SetWindowScale(4); // stride = 512
        fixture.view.UpdateScrollbarRange(FrameCount(10000));
        CHECK(scrollBar->singleStep() == 5120);
        CHECK(scrollBar->pageStep() == (512 * fixture.view.viewport()->height()));
    }
}