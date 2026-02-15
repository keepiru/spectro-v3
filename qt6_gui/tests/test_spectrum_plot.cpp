// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "controllers/spectrogram_controller.h"
#include "fft_window.h"
#include "models/audio_buffer.h"
#include "models/settings.h"
#include "tests/spectrogram_controller_test_fixture.h"
#include "views/spectrum_plot.h"
#include <QImage>
#include <QLine>
#include <QObject>
#include <QPainter>
#include <QPoint>
#include <QPolygon>
#include <QWidget>
#include <audio_types.h>
#include <catch2/catch_all.hpp>
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

/// @brief Testable subclass of SpectrumPlot that exposes private members for testing
class TestableSpectrumPlot : public SpectrumPlot
{
  public:
    using SpectrumPlot::SpectrumPlot;

    // Expose private types for testing
    using SpectrumPlot::DecibelScaleParameters;
    using SpectrumPlot::Marker;

    // Expose private methods for testing
    using SpectrumPlot::CalculateDecibelScaleParameters;
    using SpectrumPlot::ComputeCrosshair;
    using SpectrumPlot::ComputePoints;
    using SpectrumPlot::GenerateDecibelScaleMarkers;
    using SpectrumPlot::GetDecibels;
};

/// @brief Common test fixture for SpectrumPlot tests
struct SpectrumPlotTestFixture : public SpectrogramControllerTestFixture
{
    TestableSpectrumPlot plot{ controller };
};

} // namespace

TEST_CASE("SpectrumPlot constructor", "[spectrum_plot]")
{
    const SpectrumPlotTestFixture fixture;
    REQUIRE(fixture.plot.minimumWidth() > 0);
    REQUIRE(fixture.plot.minimumHeight() > 0);
}

TEST_CASE("SpectrumPlot is widget", "[spectrum_plot]")
{
    SpectrumPlotTestFixture fixture;
    REQUIRE(qobject_cast<QWidget*>(&fixture.plot) != nullptr);
}

TEST_CASE("SpectrumPlot::GetDecibels", "[spectrum_plot]")
{
    SpectrumPlotTestFixture fixture;
    fixture.settings.SetFFTSettings(8, FFTWindow::Type::Rectangular);
    fixture.settings.SetWindowScale(2); // Stride 4

    auto fillBuffer = [&fixture](size_t aFrameCount) {
        fixture.audio_buffer.Reset(2, 44100);
        for (size_t i = 0; i < aFrameCount; ++i) {
            fixture.audio_buffer.AddSamples(
              { static_cast<float>(2 * i), static_cast<float>((2 * i) + 1) });
        }
    };

    SECTION("returns correct values for last available stride")
    {
        fillBuffer(15);

        // Check our assumptions.  15 frames loaded:
        const FrameCount kAvailableFrameCount = fixture.controller.GetAvailableFrameCount();
        CHECK(kAvailableFrameCount == FrameCount(15));

        // With window size 8 and stride 4, top of window should start at frame 4
        const FramePosition kTopFrame =
          fixture.controller.CalculateTopOfWindow(kAvailableFrameCount.AsPosition());
        CHECK(kTopFrame == FramePosition{ 4 });
        // Therefore, FFT window should cover frames 4-8:
        const std::vector<std::vector<float>> want = { { 8, 10, 12, 14, 16 },
                                                       { 9, 11, 13, 15, 17 } };
        CHECK(fixture.plot.GetDecibels(0) == want[0]);
        CHECK(fixture.plot.GetDecibels(1) == want[1]);
    }

    SECTION("returns zeroed vector if insufficient samples")
    {
        fillBuffer(7); // Less than window size of 8
        const std::vector<float> want = { 0, 0, 0, 0, 0 };
        CHECK(fixture.plot.GetDecibels(0) == want);
        CHECK(fixture.plot.GetDecibels(1) == want);
    }

    SECTION("throws out_of_range for invalid channel")
    {
        fixture.audio_buffer.Reset(1, 44100);
        REQUIRE_THROWS_MATCHES(fixture.plot.GetDecibels(1),
                               std::out_of_range,
                               Catch::Matchers::Message("Channel index out of range"));
    }
}

TEST_CASE("SpectrumPlot::ComputePoints", "[spectrum_plot]")
{
    SpectrumPlotTestFixture fixture;

    SECTION("computes correct points for given decibel values")
    {
        const std::vector<float> decibels = { -100.0f, -50.0f, 0.0f, 50.0f, 100.0f };
        fixture.settings.SetApertureFloorDecibels(-100.0f);
        fixture.settings.SetApertureCeilingDecibels(100.0f);

        const QPolygonF have = fixture.plot.ComputePoints(decibels, 10, 100);
        REQUIRE(static_cast<size_t>(have.size()) == decibels.size());
        CHECK(have[0] == QPointF(0.0f, 100.0f)); // -100 dB -> bottom
        CHECK(have[1] == QPointF(1.0f, 75.0f));  // -50 dB
        CHECK(have[2] == QPointF(2.0f, 50.0f));  // 0 dB
        CHECK(have[3] == QPointF(3.0f, 25.0f));  // 50 dB
        CHECK(have[4] == QPointF(4.0f, 0.0f));   // 100 dB -> top
    }

    SECTION("does not include points outside the given width")
    {
        const std::vector<float> decibels = { -100.0f, -50.0f, 0.0f, 50.0f, 100.0f };
        fixture.settings.SetApertureFloorDecibels(-100.0f);
        fixture.settings.SetApertureCeilingDecibels(100.0f);

        const QPolygonF have = fixture.plot.ComputePoints(decibels, 3, 100);
        REQUIRE(have.size() == 3);
        CHECK(have[0] == QPointF(0.0f, 100.0f));
        CHECK(have[1] == QPointF(1.0f, 75.0f));
        CHECK(have[2] == QPointF(2.0f, 50.0f));
    }

    SECTION("returns empty polygon if decibel range is zero")
    {
        const std::vector<float> decibels = { -50.0f, -50.0f, -50.0f };
        fixture.settings.SetApertureFloorDecibels(-50.0f);
        fixture.settings.SetApertureCeilingDecibels(-50.0f);

        const QPolygonF have = fixture.plot.ComputePoints(decibels, 10, 100);
        REQUIRE(have.empty());
    }
}

TEST_CASE("SpectrumPlot::ComputeDecibelScaleMarkers", "[spectrum_plot]")
{
    SpectrumPlotTestFixture fixture;

    SECTION("returns correct markers for given height and width")
    {
        fixture.settings.SetApertureFloorDecibels(-60.0f);
        fixture.settings.SetApertureCeilingDecibels(0.0f);

        const auto params = fixture.plot.CalculateDecibelScaleParameters(120);
        const TestableSpectrumPlot::DecibelScaleParameters wantParams = {
            .aperture_floor_decibels = -60.0f,
            .aperture_ceiling_decibels = 0.0f,
            .pixels_per_decibel = 2.0f,
            .decibel_step = 10,
            .top_marker_decibels = 0.0f,
            .marker_count = 7,
        };
        REQUIRE(params == wantParams);

        const std::vector<TestableSpectrumPlot::Marker> want = {
            { .line = QLine(190, 0, 200, 0), .rect = QRect(165, -5, 20, 10), .text = "0" },
            { .line = QLine(190, 20, 200, 20), .rect = QRect(165, 15, 20, 10), .text = "-10" },
            { .line = QLine(190, 40, 200, 40), .rect = QRect(165, 35, 20, 10), .text = "-20" },
            { .line = QLine(190, 60, 200, 60), .rect = QRect(165, 55, 20, 10), .text = "-30" },
            { .line = QLine(190, 80, 200, 80), .rect = QRect(165, 75, 20, 10), .text = "-40" },
            { .line = QLine(190, 100, 200, 100), .rect = QRect(165, 95, 20, 10), .text = "-50" },
            { .line = QLine(190, 120, 200, 120), .rect = QRect(165, 115, 20, 10), .text = "-60" },
        };
        REQUIRE(fixture.plot.GenerateDecibelScaleMarkers(params, 200) == want);
    }

    SECTION("handles zero height")
    {
        fixture.settings.SetApertureFloorDecibels(-60.0f);
        fixture.settings.SetApertureCeilingDecibels(0.0f);

        const auto params = fixture.plot.CalculateDecibelScaleParameters(0);
        const TestableSpectrumPlot::DecibelScaleParameters wantParams = {
            .aperture_floor_decibels = -60.0f,
            .aperture_ceiling_decibels = 0.0f,
            .pixels_per_decibel = 0.0f,
            .decibel_step = 50,
            .top_marker_decibels = 0.0f,
            .marker_count = 2,
        };
        REQUIRE(params == wantParams);

        const std::vector<TestableSpectrumPlot::Marker> want = {
            { .line = QLine(190, 0, 200, 0), .rect = QRect(165, -5, 20, 10), .text = "0" },
            { .line = QLine(190, 0, 200, 0), .rect = QRect(165, -5, 20, 10), .text = "-50" },
        };
        REQUIRE(fixture.plot.GenerateDecibelScaleMarkers(params, 200) == want);
    }

    SECTION("handles small aperture")
    {
        fixture.settings.SetApertureFloorDecibels(-1.0f);
        fixture.settings.SetApertureCeilingDecibels(1.0f);

        const auto params = fixture.plot.CalculateDecibelScaleParameters(120);
        const TestableSpectrumPlot::DecibelScaleParameters wantParams = {
            .aperture_floor_decibels = -1.0f,
            .aperture_ceiling_decibels = 1.0f,
            .pixels_per_decibel = 60.0f,
            .decibel_step = 1,
            .top_marker_decibels = 1.0f,
            .marker_count = 3,
        };
        REQUIRE(params == wantParams);

        const std::vector<TestableSpectrumPlot::Marker> want = {
            { .line = QLine(190, 0, 200, 0), .rect = QRect(165, -5, 20, 10), .text = "1" },
            { .line = QLine(190, 60, 200, 60), .rect = QRect(165, 55, 20, 10), .text = "0" },
            { .line = QLine(190, 120, 200, 120), .rect = QRect(165, 115, 20, 10), .text = "-1" },
        };
        REQUIRE(fixture.plot.GenerateDecibelScaleMarkers(params, 200) == want);
    }

    SECTION("handles inverted aperture")
    {
        fixture.settings.SetApertureFloorDecibels(0.0f);
        fixture.settings.SetApertureCeilingDecibels(-60.0f);

        const auto params = fixture.plot.CalculateDecibelScaleParameters(120);
        const TestableSpectrumPlot::DecibelScaleParameters wantParams = {
            .aperture_floor_decibels = 0.0f,
            .aperture_ceiling_decibels = -60.0f,
            .pixels_per_decibel = -2.0f,
            .decibel_step = -10,
            .top_marker_decibels = -60.0f,
            .marker_count = 7,
        };
        REQUIRE(params == wantParams);

        const std::vector<TestableSpectrumPlot::Marker> want = {
            { .line = QLine(190, 0, 200, 0), .rect = QRect(165, -5, 20, 10), .text = "-60" },
            { .line = QLine(190, 20, 200, 20), .rect = QRect(165, 15, 20, 10), .text = "-50" },
            { .line = QLine(190, 40, 200, 40), .rect = QRect(165, 35, 20, 10), .text = "-40" },
            { .line = QLine(190, 60, 200, 60), .rect = QRect(165, 55, 20, 10), .text = "-30" },
            { .line = QLine(190, 80, 200, 80), .rect = QRect(165, 75, 20, 10), .text = "-20" },
            { .line = QLine(190, 100, 200, 100), .rect = QRect(165, 95, 20, 10), .text = "-10" },
            { .line = QLine(190, 120, 200, 120), .rect = QRect(165, 115, 20, 10), .text = "0" },
        };
        REQUIRE(fixture.plot.GenerateDecibelScaleMarkers(params, 200) == want);
    }

    SECTION("handles equal floor and ceiling aperture")
    {
        fixture.settings.SetApertureFloorDecibels(-50.0f);
        fixture.settings.SetApertureCeilingDecibels(-50.0f);

        const auto params = fixture.plot.CalculateDecibelScaleParameters(120);
        const TestableSpectrumPlot::DecibelScaleParameters wantParams = {
            .aperture_floor_decibels = -50.0f,
            .aperture_ceiling_decibels = -50.0f,
            .pixels_per_decibel = 0.0f,
            .decibel_step = 0,
            .top_marker_decibels = 0.0f,
            .marker_count = 0,
        };
        REQUIRE(params == wantParams);

        const std::vector<TestableSpectrumPlot::Marker> want = {};
        REQUIRE(fixture.plot.GenerateDecibelScaleMarkers(params, 200) == want);
    }
}

TEST_CASE("SpectrumPlot::CalculateDecibelScaleParameters", "[spectrum_plot]")
{
    SpectrumPlotTestFixture fixture;

    // We'll just do a basic smoke test here. Detailed correctness is
    // verified in GenerateDecibelScaleMarkers() tests.

    SECTION("computes correct parameters for normal aperture")
    {
        fixture.settings.SetApertureFloorDecibels(-60.0f);
        fixture.settings.SetApertureCeilingDecibels(0.0f);
        const TestableSpectrumPlot::DecibelScaleParameters want = {
            .aperture_floor_decibels = -60.0f,
            .aperture_ceiling_decibels = 0.0f,
            .pixels_per_decibel = 2.0f,
            .decibel_step = 10,
            .top_marker_decibels = 0.0f,
            .marker_count = 7,
        };
        REQUIRE(fixture.plot.CalculateDecibelScaleParameters(120) == want);
    }
}

TEST_CASE("SpectrumPlot::ComputeCrosshair", "[spectrum_plot]")
{
    SpectrumPlotTestFixture fixture;
    fixture.settings.SetFFTSettings(1024, FFTWindow::Type::Rectangular);
    fixture.settings.SetApertureFloorDecibels(-100.0f);
    fixture.settings.SetApertureCeilingDecibels(0.0f);

    SECTION("computes correct crosshair at center of widget")
    {
        const int kWidth = 512;
        const int kHeight = 200;
        const QPoint kMousePos(256, 100); // Center

        const auto markers = fixture.plot.ComputeCrosshair(kMousePos, kHeight, kWidth);
        REQUIRE(markers.size() == 2);

        // Frequency marker (vertical line)
        const auto& freqMarker = markers[0];
        CHECK(freqMarker.line == QLine(256, 0, 256, 200));
        CHECK(freqMarker.rect == QRect(261, 5, 50, 10));
        // Hz calculation: mouseX * HzPerBin = 256 * (44100/1024) = ~11025 Hz
        CHECK(freqMarker.text == "11025 Hz");

        // Decibel marker (horizontal line)
        const auto& dbMarker = markers[1];
        CHECK(dbMarker.line == QLine(0, 100, 512, 100));
        CHECK(dbMarker.rect == QRect(412, 82, 60, 15));
        // dB at center should be -50 dB (halfway between -100 and 0)
        CHECK(dbMarker.text == "-50 dB");
    }

    SECTION("computes correct crosshair at top left corner")
    {
        const int kWidth = 512;
        const int kHeight = 200;
        const QPoint kMousePos(0, 0); // Top left

        const auto markers = fixture.plot.ComputeCrosshair(kMousePos, kHeight, kWidth);
        REQUIRE(markers.size() == 2);

        // Frequency marker at x=0
        const auto& freqMarker = markers[0];
        CHECK(freqMarker.line == QLine(0, 0, 0, 200));
        CHECK(freqMarker.rect == QRect(5, 5, 50, 10));
        CHECK(freqMarker.text == "0 Hz");

        // Decibel marker at y=0 (top = max dB)
        const auto& dbMarker = markers[1];
        CHECK(dbMarker.line == QLine(0, 0, 512, 0));
        CHECK(dbMarker.rect == QRect(412, -18, 60, 15));
        CHECK(dbMarker.text == "0 dB");
    }

    SECTION("computes correct crosshair at bottom right corner")
    {
        const int kWidth = 512;
        const int kHeight = 200;
        const QPoint kMousePos(511, 199); // Bottom right

        const auto markers = fixture.plot.ComputeCrosshair(kMousePos, kHeight, kWidth);
        REQUIRE(markers.size() == 2);

        // Frequency marker at x=511
        const auto& freqMarker = markers[0];
        CHECK(freqMarker.line == QLine(511, 0, 511, 200));
        CHECK(freqMarker.rect == QRect(516, 5, 50, 10));
        CAPTURE(freqMarker);
        CHECK(freqMarker.text == "22006 Hz");

        // Decibel marker at y=199 (bottom = min dB)
        const auto& dbMarker = markers[1];
        CHECK(dbMarker.line == QLine(0, 199, 512, 199));
        CHECK(dbMarker.rect == QRect(412, 181, 60, 15));
        CHECK(dbMarker.text == "-99 dB");
    }

    SECTION("handles different aperture ranges")
    {
        fixture.settings.SetApertureFloorDecibels(-60.0f);
        fixture.settings.SetApertureCeilingDecibels(-20.0f);

        const int kWidth = 400;
        const int kHeight = 100;
        const QPoint kMousePos(200, 50); // Center

        const auto markers = fixture.plot.ComputeCrosshair(kMousePos, kHeight, kWidth);
        REQUIRE(markers.size() == 2);

        // At center of 100px height with -60 to -20 dB range:
        // Center should be -40 dB
        const auto& dbMarker = markers[1];
        CHECK(dbMarker.text == "-40 dB");
    }

    SECTION("handles inverted aperture")
    {
        fixture.settings.SetApertureFloorDecibels(0.0f);
        fixture.settings.SetApertureCeilingDecibels(-100.0f);

        const int kWidth = 512;
        const int kHeight = 200;
        const QPoint kMousePos(256, 100); // Center

        const auto markers = fixture.plot.ComputeCrosshair(kMousePos, kHeight, kWidth);
        REQUIRE(markers.size() == 2);

        // With inverted aperture, center should still compute correctly
        // normalized_y = 1 - (100/200) = 0.5
        // dB = 0 + (0.5 * -100) = -50 dB
        const auto& dbMarker = markers[1];
        CHECK(dbMarker.text == "-50 dB");
    }

    SECTION("computes correct frequency for different FFT sizes")
    {
        fixture.settings.SetFFTSettings(512, FFTWindow::Type::Rectangular);

        const int kWidth = 256;
        const int kHeight = 100;
        const QPoint kMousePos(128, 50); // Center

        const auto markers = fixture.plot.ComputeCrosshair(kMousePos, kHeight, kWidth);
        REQUIRE(markers.size() == 2);

        // Hz calculation: mouseX * HzPerBin = 128 * (44100/512) ≈ 11025 Hz
        const auto& freqMarker = markers[0];
        CHECK(freqMarker.text == "11025 Hz");
        // The exact value depends on sample rate from settings
    }
}

TEST_CASE("SpectrumPlot::DecibelScaleParameters stream output operator", "[spectrum_plot]")
{
    using DecibelScaleParameters = TestableSpectrumPlot::DecibelScaleParameters;

    SECTION("formats parameters correctly")
    {
        const DecibelScaleParameters params{ .aperture_floor_decibels = -80.0F,
                                             .aperture_ceiling_decibels = 0.0F,
                                             .pixels_per_decibel = 2.5F,
                                             .decibel_step = 10,
                                             .top_marker_decibels = -10.0F,
                                             .marker_count = 8 };
        std::ostringstream oss;
        oss << params;
        CHECK(oss.str() == "DecibelScaleParameters(aperture_floor_decibels=-80, "
                           "aperture_ceiling_decibels=0, pixels_per_decibel=2.5, "
                           "decibel_step=10, top_marker_decibels=-10, marker_count=8)");
    }

    SECTION("formats parameters with negative step")
    {
        const DecibelScaleParameters params{ .aperture_floor_decibels = 0.0F,
                                             .aperture_ceiling_decibels = -80.0F,
                                             .pixels_per_decibel = -2.5F,
                                             .decibel_step = -10,
                                             .top_marker_decibels = 0.0F,
                                             .marker_count = 8 };
        std::ostringstream oss;
        oss << params;
        CHECK(oss.str() == "DecibelScaleParameters(aperture_floor_decibels=0, "
                           "aperture_ceiling_decibels=-80, pixels_per_decibel=-2.5, "
                           "decibel_step=-10, top_marker_decibels=0, marker_count=8)");
    }
}

TEST_CASE("SpectrumPlot::Marker stream output operator", "[spectrum_plot]")
{
    using Marker = TestableSpectrumPlot::Marker;

    SECTION("formats marker correctly")
    {
        const Marker marker{ .line = QLine(10, 20, 30, 40),
                             .rect = QRect(50, 60, 70, 80),
                             .text = "Test Label" };
        std::ostringstream oss;
        oss << marker;
        CHECK(oss.str() == "Marker(line=(10, 20, 30, 40),rect=(50, 60, 70, 80),'Test Label')");
    }

    SECTION("formats marker with empty text")
    {
        const Marker marker{ .line = QLine(0, 0, 100, 100),
                             .rect = QRect(5, 10, 15, 20),
                             .text = "" };
        std::ostringstream oss;
        oss << marker;
        CHECK(oss.str() == "Marker(line=(0, 0, 100, 100),rect=(5, 10, 15, 20),'')");
    }
}

TEST_CASE("SpectrumPlot::paintEvent", "[spectrum_plot]")
{
    SpectrumPlotTestFixture fixture;
    QImage image(800, 400, QImage::Format_RGB32);
    QPainter painter(&image);

    SECTION("renders without errors for various FFT sizes")
    {
        // Test with different FFT sizes to ensure paint event handles all cases
        const FFTSize kFFTSize = GENERATE(from_range(Settings::KValidFFTSizes));
        fixture.settings.SetFFTSettings(kFFTSize, FFTWindow::Type::Hann);

        CHECK_NOTHROW(fixture.plot.render(&painter));
    }

    SECTION("renders with different widget sizes")
    {
        const std::vector<std::pair<int, int>> sizes = {
            { 100, 100 }, { 400, 200 }, { 800, 600 }, { 1920, 1080 }
        };

        for (const auto& [width, height] : sizes) {
            fixture.plot.setFixedSize(width, height);

            QImage localImage(width, height, QImage::Format_RGB32);
            QPainter localPainter(&localImage);
            CHECK_NOTHROW(fixture.plot.render(&localPainter));
        }
    }

    SECTION("renders with various channel counts")
    {
        // Test with different channel counts
        const ChannelCount kChannelCount = GENERATE(1, 2, 3, 4, 5, 6);
        fixture.audio_buffer.Reset(kChannelCount, 44100);

        // Add some sample data
        const std::vector<float> kSamples(static_cast<size_t>(kChannelCount) * 2048, 0.0f);
        fixture.audio_buffer.AddSamples(kSamples);
        CHECK_NOTHROW(fixture.plot.render(&painter));
    }

    SECTION("renders with different aperture ranges")
    {
        const std::vector<std::pair<float, float>> apertureRanges = {
            { -120.0f, 0.0f },
            { -80.0f, -20.0f },
            { -60.0f, 0.0f },
            { 0.0f, -60.0f },  // Inverted
            { -50.0f, -50.0f } // Equal (edge case)
        };

        for (const auto& [floorDb, ceilingDb] : apertureRanges) {
            fixture.settings.SetApertureFloorDecibels(floorDb);
            fixture.settings.SetApertureCeilingDecibels(ceilingDb);

            CHECK_NOTHROW(fixture.plot.render(&painter));
        }
    }
}