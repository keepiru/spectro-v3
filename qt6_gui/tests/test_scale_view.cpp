// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "audio_types.h"
#include "controllers/spectrogram_controller.h"
#include "fft_window.h"
#include "models/audio_buffer.h"
#include "models/settings.h"
#include "views/scale_view.h"
#include <QImage>
#include <QPainter>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstddef>
#include <mock_fft_processor.h>
#include <optional>
#include <sstream>
#include <vector>

/// @brief Test fixture for ScaleView
class TestableScaleView : public ScaleView
{
  public:
    using ScaleView::ScaleView;

    // Expose protected types for testing
    using ScaleView::TickMark;

    // Expose protected methods for testing
    using ScaleView::CalculateTickMarks;
};

TEST_CASE("ScaleView constructor", "[scale_view]")
{
    const Settings settings;
    const AudioBuffer audioBuffer;
    const SpectrogramController controller(settings, audioBuffer);
    const ScaleView view(controller);

    REQUIRE(view.height() == 20);
    REQUIRE(view.minimumHeight() == 20);
    REQUIRE(view.maximumHeight() == 20);
}

TEST_CASE("ScaleView::paintEvent", "[scale_view]")
{
    Settings settings;
    const AudioBuffer audioBuffer;
    const SpectrogramController controller(
      settings, audioBuffer, MockFFTProcessor::GetFactory(), nullptr);
    TestableScaleView view(controller);

    SECTION("renders without errors for various FFT sizes")
    {
        // Test with different FFT sizes to ensure paint event handles all cases
        const FFTSize kFFTSize = GENERATE(512, 1024, 2048, 4096, 8192, 16384);

        settings.SetFFTSettings(kFFTSize, FFTWindow::Type::Hann);

        // Create an image to paint on
        QImage image(1024, 20, QImage::Format_RGB32);

        // Trigger paint event by painting to the image
        QPainter painter(&image);
        CHECK_NOTHROW(view.render(&painter));
    }

    SECTION("renders with different widget widths")
    {
        settings.SetFFTSettings(2048, FFTWindow::Type::Rectangular);

        const int kWidth = GENERATE(10, 100, 512, 1024, 2048);
        view.setFixedWidth(kWidth);

        QImage image(kWidth, 20, QImage::Format_RGB32);

        QPainter painter(&image);
        CHECK_NOTHROW(view.render(&painter));
    }
}

TEST_CASE("ScaleView::CalculateTickMarks", "[scale_view]")
{
    Settings settings;
    AudioBuffer const audioBuffer;
    const SpectrogramController controller(
      settings, audioBuffer, MockFFTProcessor::GetFactory(), nullptr);
    const TestableScaleView view(controller);

    // Set up FFT to have known Hz per bin
    // With sample rate 44100 and FFT size 2048: Hz per bin = 44100 / 2048 ≈ 21.53 Hz/bin
    settings.SetFFTSettings(2048, FFTWindow::Type::Rectangular);

    SECTION("handles zero width")
    {
        CHECK(view.CalculateTickMarks(0).empty());
    }

    SECTION("generates no tick marks for narrow width")
    {
        // with transform 2048, minimum width to show first tick at 200 Hz is 10 pixels
        CHECK(view.CalculateTickMarks(9).empty());
        CHECK(view.CalculateTickMarks(10).size() == 1);
    }

    SECTION("handles very large width")
    {
        const auto have = view.CalculateTickMarks(100000);
        CHECK(have.size() == 10766);
    }

    SECTION("generates tick marks at 400 Hz intervals for 1024 transform size")
    {
        settings.SetFFTSettings(1024, FFTWindow::Type::Hann);

        // Width of 1024 bins * 43.07 Hz/bin ≈ 44106 Hz max frequency
        // Should get ticks at 400, 800, 1200, ... Hz
        const auto have = view.CalculateTickMarks(1024);
        CHECK(have.size() == 110);
        CHECK(have.at(0) == TestableScaleView::TickMark{ 9, {} });         // 400 Hz
        CHECK(have.at(1) == TestableScaleView::TickMark{ 18, {} });        // 800 Hz
        CHECK(have.at(2) == TestableScaleView::TickMark{ 27, {} });        // 1200 Hz
        CHECK(have.at(3) == TestableScaleView::TickMark{ 37, {} });        // 1600 Hz
        CHECK(have.at(4) == TestableScaleView::TickMark{ 46, 2000 });      // 2000 Hz
        CHECK(have.at(5) == TestableScaleView::TickMark{ 55, {} });        // 2400 Hz
        CHECK(have.at(107) == TestableScaleView::TickMark{ 1003, {} });    // 43200 Hz
        CHECK(have.at(108) == TestableScaleView::TickMark{ 1012, {} });    // 43600 Hz
        CHECK(have.at(109) == TestableScaleView::TickMark{ 1021, 44000 }); // 44000 Hz
    }

    SECTION("generates tick marks at 200 Hz intervals for 2048 transform size")
    {
        // Width of 1024 bins * 21.53 Hz/bin ≈ 22053 Hz max frequency
        // Should get ticks at 200, 400, 600, ... Hz
        const auto have = view.CalculateTickMarks(1024);
        CHECK(have.size() == 110);
        CHECK(have.at(0) == TestableScaleView::TickMark{ 9, {} });         // 200 Hz
        CHECK(have.at(1) == TestableScaleView::TickMark{ 18, {} });        // 400 Hz
        CHECK(have.at(2) == TestableScaleView::TickMark{ 27, {} });        // 600 Hz
        CHECK(have.at(3) == TestableScaleView::TickMark{ 37, {} });        // 800 Hz
        CHECK(have.at(4) == TestableScaleView::TickMark{ 46, 1000 });      // 1000 Hz
        CHECK(have.at(5) == TestableScaleView::TickMark{ 55, {} });        // 1200 Hz
        CHECK(have.at(107) == TestableScaleView::TickMark{ 1003, {} });    // 21600 Hz
        CHECK(have.at(108) == TestableScaleView::TickMark{ 1012, {} });    // 21800 Hz
        CHECK(have.at(109) == TestableScaleView::TickMark{ 1021, 22000 }); // 22000 Hz
    }

    SECTION("generates tick marks at 100 Hz intervals for 4096 transform size")
    {
        settings.SetFFTSettings(4096, FFTWindow::Type::Hann);

        // Width of 1024 bins * 10.77 Hz/bin ≈ 11059 Hz max frequency
        // Should get ticks at 100, 200, 300, ... Hz
        const auto have = view.CalculateTickMarks(1024);
        CHECK(have.size() == 110);
        CHECK(have.at(0) == TestableScaleView::TickMark{ 9, {} });         // 100 Hz
        CHECK(have.at(1) == TestableScaleView::TickMark{ 18, {} });        // 200 Hz
        CHECK(have.at(2) == TestableScaleView::TickMark{ 27, {} });        // 300 Hz
        CHECK(have.at(3) == TestableScaleView::TickMark{ 37, {} });        // 400 Hz
        CHECK(have.at(4) == TestableScaleView::TickMark{ 46, 500 });       // 500 Hz
        CHECK(have.at(5) == TestableScaleView::TickMark{ 55, {} });        // 600 Hz
        CHECK(have.at(107) == TestableScaleView::TickMark{ 1003, {} });    // 10800 Hz
        CHECK(have.at(108) == TestableScaleView::TickMark{ 1012, {} });    // 10900 Hz
        CHECK(have.at(109) == TestableScaleView::TickMark{ 1021, 11000 }); // 11000 Hz
    }

    SECTION("generates tick marks at 50 Hz intervals for 8192 transform size")
    {
        settings.SetFFTSettings(8192, FFTWindow::Type::Hann);

        // Width of 1024 bins * 5.38 Hz/bin ≈ 5513 Hz max frequency
        // Should get ticks at 50, 100, 150, ... Hz
        const auto have = view.CalculateTickMarks(1024);
        CHECK(have.size() == 110);
        CHECK(have.at(0) == TestableScaleView::TickMark{ 9, {} });        // 50 Hz
        CHECK(have.at(1) == TestableScaleView::TickMark{ 18, {} });       // 100 Hz
        CHECK(have.at(2) == TestableScaleView::TickMark{ 27, {} });       // 150 Hz
        CHECK(have.at(3) == TestableScaleView::TickMark{ 37, {} });       // 200 Hz
        CHECK(have.at(4) == TestableScaleView::TickMark{ 46, 250 });      // 250 Hz
        CHECK(have.at(5) == TestableScaleView::TickMark{ 55, {} });       // 300 Hz
        CHECK(have.at(107) == TestableScaleView::TickMark{ 1003, {} });   // 5400 Hz
        CHECK(have.at(108) == TestableScaleView::TickMark{ 1012, {} });   // 5450 Hz
        CHECK(have.at(109) == TestableScaleView::TickMark{ 1021, 5500 }); // 5500 Hz
    }

    SECTION("generates tick marks at 200 Hz intervals for default transform size (16384)")
    {
        settings.SetFFTSettings(16384, FFTWindow::Type::Hann);

        // Width of 1024 bins * 2.69 Hz/bin ≈ 2756 Hz max frequency
        // Should get ticks at 200, 400, 600, ... Hz (default case)
        const auto have = view.CalculateTickMarks(1024);
        CHECK(have.size() == 13);
        CHECK(have.at(0) == TestableScaleView::TickMark{ 74, {} });    // 200 Hz
        CHECK(have.at(1) == TestableScaleView::TickMark{ 148, {} });   // 400 Hz
        CHECK(have.at(2) == TestableScaleView::TickMark{ 222, {} });   // 600 Hz
        CHECK(have.at(3) == TestableScaleView::TickMark{ 297, {} });   // 800 Hz
        CHECK(have.at(4) == TestableScaleView::TickMark{ 371, 1000 }); // 1000 Hz (tick 5)
        CHECK(have.at(5) == TestableScaleView::TickMark{ 445, {} });   // 1200 Hz
        CHECK(have.at(9) == TestableScaleView::TickMark{ 743, 2000 }); // 2000 Hz (tick 10)
        CHECK(have.at(10) == TestableScaleView::TickMark{ 817, {} });  // 2200 Hz
        CHECK(have.at(11) == TestableScaleView::TickMark{ 891, {} });  // 2400 Hz
        CHECK(have.at(12) == TestableScaleView::TickMark{ 965, {} });  // 2600 Hz
    }

    SECTION("marks every 5th tick as long tick with label")
    {
        const auto tickMarks = view.CalculateTickMarks(1024);

        // Check pattern: ticks at 200, 400, 600, 800, 1000, ...
        // Every 5th tick (1000, 2000, 3000, ...) should be long
        for (size_t i = 0; i < tickMarks.size(); ++i) {
            const size_t tickNumber = i + 1;
            const size_t expectedFreq = tickNumber * 200;

            if (tickNumber % 5 == 0) {
                // Should be a long tick with label
                REQUIRE(tickMarks[i].label.has_value());
                REQUIRE(tickMarks[i].label.value_or(0) == expectedFreq);
            } else {
                // Should be a short tick without label
                REQUIRE(tickMarks[i].label.has_value() == false);
            }
        }
    }
}

TEST_CASE("ScaleView::TickMark stream output operator", "[scale_view]")
{
    using TickMark = TestableScaleView::TickMark;

    SECTION("formats tick mark with label")
    {
        const TickMark tick{ 42, 1000 };
        std::ostringstream oss;
        oss << tick;
        CHECK(oss.str() == "TickMark{position: 42, label: 1000}");
    }

    SECTION("formats tick mark without label")
    {
        const TickMark tick{ 100, std::nullopt };
        std::ostringstream oss;
        oss << tick;
        CHECK(oss.str() == "TickMark{position: 100, label: none}");
    }
}