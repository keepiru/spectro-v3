#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "views/spectrum_plot.h"
#include <audio_types.h>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <mock_fft_processor.h>
#include <vector>

TEST_CASE("SpectrumPlot constructor", "[spectrum_plot]")
{
    const Settings settings;
    const AudioBuffer audioBuffer;
    const SpectrogramController controller(settings, audioBuffer);
    const SpectrumPlot plot(controller);
    REQUIRE(plot.minimumWidth() > 0);
    REQUIRE(plot.minimumHeight() > 0);
}

TEST_CASE("SpectrumPlot is widget", "[spectrum_plot]")
{
    const Settings settings;
    const AudioBuffer audioBuffer;
    const SpectrogramController controller(settings, audioBuffer);
    SpectrumPlot plot(controller);
    REQUIRE(qobject_cast<QWidget*>(&plot) != nullptr);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity) macro expansion
TEST_CASE("SpectrumPlot::GetDecibels", "[spectrum_plot]")
{
    Settings settings;
    AudioBuffer audioBuffer;
    const SpectrogramController controller(settings, audioBuffer, MockFFTProcessor::GetFactory());
    const SpectrumPlot plot(controller);
    settings.SetFFTSettings(8, FFTWindow::Type::Rectangular);
    settings.SetWindowScale(2); // Stride 4

    auto fillBuffer = [&audioBuffer](size_t aFrameCount) {
        audioBuffer.Reset(2, 44100);
        for (size_t i = 0; i < aFrameCount; ++i) {
            audioBuffer.AddSamples({ static_cast<float>(2 * i), static_cast<float>((2 * i) + 1) });
        }
    };

    SECTION("returns correct values for last available stride")
    {
        fillBuffer(15);

        // Check our assumptions.  15 frames loaded:
        const FrameCount kAvailableFrameCount = controller.GetAvailableFrameCount();
        CHECK(kAvailableFrameCount == 15);

        // With window size 8 and stride 4, top of window should start at frame 4
        const FrameOffset kTopFrame =
          controller.CalculateTopOfWindow(kAvailableFrameCount.AsOffset());
        CHECK(kTopFrame == 4);
        // Therefore, FFT window should cover frames 4-8:
        const std::vector<std::vector<float>> want = { { 8, 10, 12, 14, 16 },
                                                       { 9, 11, 13, 15, 17 } };
        CHECK(plot.GetDecibels(0) == want[0]);
        CHECK(plot.GetDecibels(1) == want[1]);
    }

    SECTION("returns zeroed vector if insufficient samples")
    {
        fillBuffer(7); // Less than window size of 8
        const std::vector<float> want = { 0, 0, 0, 0, 0 };
        CHECK(plot.GetDecibels(0) == want);
        CHECK(plot.GetDecibels(1) == want);
    }

    SECTION("throws out_of_range for invalid channel")
    {
        audioBuffer.Reset(1, 44100);
        REQUIRE_THROWS_MATCHES(plot.GetDecibels(1),
                               std::out_of_range,
                               Catch::Matchers::Message("Channel index out of range"));
    }
}

TEST_CASE("SpectrumPlot::ComputePoints", "[spectrum_plot]")
{
    Settings settings;
    const AudioBuffer audioBuffer;
    const SpectrogramController controller(settings, audioBuffer);
    const SpectrumPlot plot(controller);

    SECTION("computes correct points for given decibel values")
    {
        const std::vector<float> decibels = { -100.0f, -50.0f, 0.0f, 50.0f, 100.0f };
        settings.SetApertureMinDecibels(-100.0f);
        settings.SetApertureMaxDecibels(100.0f);

        const QPolygonF have = plot.ComputePoints(decibels, 10, 100);
        REQUIRE(have.size() == decibels.size());
        CHECK(have[0] == QPointF(0.0f, 100.0f)); // -100 dB -> bottom
        CHECK(have[1] == QPointF(1.0f, 75.0f));  // -50 dB
        CHECK(have[2] == QPointF(2.0f, 50.0f));  // 0 dB
        CHECK(have[3] == QPointF(3.0f, 25.0f));  // 50 dB
        CHECK(have[4] == QPointF(4.0f, 0.0f));   // 100 dB -> top
    }

    SECTION("does not include points outside the given width")
    {
        const std::vector<float> decibels = { -100.0f, -50.0f, 0.0f, 50.0f, 100.0f };
        settings.SetApertureMinDecibels(-100.0f);
        settings.SetApertureMaxDecibels(100.0f);

        const QPolygonF have = plot.ComputePoints(decibels, 3, 100);
        REQUIRE(have.size() == 3);
        CHECK(have[0] == QPointF(0.0f, 100.0f));
        CHECK(have[1] == QPointF(1.0f, 75.0f));
        CHECK(have[2] == QPointF(2.0f, 50.0f));
    }

    SECTION("returns empty polygon if decibel range is zero")
    {
        const std::vector<float> decibels = { -50.0f, -50.0f, -50.0f };
        settings.SetApertureMinDecibels(-50.0f);
        settings.SetApertureMaxDecibels(-50.0f);

        const QPolygonF have = plot.ComputePoints(decibels, 10, 100);
        REQUIRE(have.empty());
    }
}

TEST_CASE("SpectrumPlot::ComputeDecibelScaleMarkers", "[spectrum_plot]")
{
    Settings settings;
    const AudioBuffer audioBuffer;
    const SpectrogramController controller(settings, audioBuffer);
    const SpectrumPlot plot(controller);

    SECTION("returns correct markers for given height and width")
    {
        settings.SetApertureMinDecibels(-60.0f);
        settings.SetApertureMaxDecibels(0.0f);

        const auto params = plot.CalculateDecibelScaleParameters(120);
        const SpectrumPlot::DecibelScaleParameters wantParams = {
            .aperture_min_decibels = -60.0f,
            .aperture_max_decibels = 0.0f,
            .pixels_per_decibel = 2.0f,
            .decibel_step = 10,
            .top_marker_decibels = 0.0f,
            .marker_count = 7,
        };
        REQUIRE(params == wantParams);

        const std::vector<SpectrumPlot::Marker> want = {
            { .line = QLine(190, 0, 200, 0), .rect = QRect(165, -5, 20, 10), .text = "0" },
            { .line = QLine(190, 20, 200, 20), .rect = QRect(165, 15, 20, 10), .text = "-10" },
            { .line = QLine(190, 40, 200, 40), .rect = QRect(165, 35, 20, 10), .text = "-20" },
            { .line = QLine(190, 60, 200, 60), .rect = QRect(165, 55, 20, 10), .text = "-30" },
            { .line = QLine(190, 80, 200, 80), .rect = QRect(165, 75, 20, 10), .text = "-40" },
            { .line = QLine(190, 100, 200, 100), .rect = QRect(165, 95, 20, 10), .text = "-50" },
            { .line = QLine(190, 120, 200, 120), .rect = QRect(165, 115, 20, 10), .text = "-60" },
        };
        REQUIRE(plot.GenerateDecibelScaleMarkers(params, 200) == want);
    }

    SECTION("handles zero height")
    {
        settings.SetApertureMinDecibels(-60.0f);
        settings.SetApertureMaxDecibels(0.0f);

        const auto params = plot.CalculateDecibelScaleParameters(0);
        const SpectrumPlot::DecibelScaleParameters wantParams = {
            .aperture_min_decibels = -60.0f,
            .aperture_max_decibels = 0.0f,
            .pixels_per_decibel = 0.0f,
            .decibel_step = 50,
            .top_marker_decibels = 0.0f,
            .marker_count = 2,
        };
        REQUIRE(params == wantParams);

        const std::vector<SpectrumPlot::Marker> want = {
            { .line = QLine(190, 0, 200, 0), .rect = QRect(165, -5, 20, 10), .text = "0" },
            { .line = QLine(190, 0, 200, 0), .rect = QRect(165, -5, 20, 10), .text = "-50" },
        };
        REQUIRE(plot.GenerateDecibelScaleMarkers(params, 200) == want);
    }

    SECTION("handles small aperture")
    {
        settings.SetApertureMinDecibels(-1.0f);
        settings.SetApertureMaxDecibels(1.0f);

        const auto params = plot.CalculateDecibelScaleParameters(120);
        const SpectrumPlot::DecibelScaleParameters wantParams = {
            .aperture_min_decibels = -1.0f,
            .aperture_max_decibels = 1.0f,
            .pixels_per_decibel = 60.0f,
            .decibel_step = 1,
            .top_marker_decibels = 1.0f,
            .marker_count = 3,
        };
        REQUIRE(params == wantParams);

        const std::vector<SpectrumPlot::Marker> want = {
            { .line = QLine(190, 0, 200, 0), .rect = QRect(165, -5, 20, 10), .text = "1" },
            { .line = QLine(190, 60, 200, 60), .rect = QRect(165, 55, 20, 10), .text = "0" },
            { .line = QLine(190, 120, 200, 120), .rect = QRect(165, 115, 20, 10), .text = "-1" },
        };
        REQUIRE(plot.GenerateDecibelScaleMarkers(params, 200) == want);
    }

    SECTION("handles inverted aperture")
    {
        settings.SetApertureMinDecibels(0.0f);
        settings.SetApertureMaxDecibels(-60.0f);

        const auto params = plot.CalculateDecibelScaleParameters(120);
        const SpectrumPlot::DecibelScaleParameters wantParams = {
            .aperture_min_decibels = 0.0f,
            .aperture_max_decibels = -60.0f,
            .pixels_per_decibel = -2.0f,
            .decibel_step = -10,
            .top_marker_decibels = -60.0f,
            .marker_count = 7,
        };
        REQUIRE(params == wantParams);

        const std::vector<SpectrumPlot::Marker> want = {
            { .line = QLine(190, 0, 200, 0), .rect = QRect(165, -5, 20, 10), .text = "-60" },
            { .line = QLine(190, 20, 200, 20), .rect = QRect(165, 15, 20, 10), .text = "-50" },
            { .line = QLine(190, 40, 200, 40), .rect = QRect(165, 35, 20, 10), .text = "-40" },
            { .line = QLine(190, 60, 200, 60), .rect = QRect(165, 55, 20, 10), .text = "-30" },
            { .line = QLine(190, 80, 200, 80), .rect = QRect(165, 75, 20, 10), .text = "-20" },
            { .line = QLine(190, 100, 200, 100), .rect = QRect(165, 95, 20, 10), .text = "-10" },
            { .line = QLine(190, 120, 200, 120), .rect = QRect(165, 115, 20, 10), .text = "0" },
        };
        REQUIRE(plot.GenerateDecibelScaleMarkers(params, 200) == want);
    }

    SECTION("handles equal min and max aperture")
    {
        settings.SetApertureMinDecibels(-50.0f);
        settings.SetApertureMaxDecibels(-50.0f);

        const auto params = plot.CalculateDecibelScaleParameters(120);
        const SpectrumPlot::DecibelScaleParameters wantParams = {
            .aperture_min_decibels = -50.0f,
            .aperture_max_decibels = -50.0f,
            .pixels_per_decibel = 0.0f,
            .decibel_step = 0,
            .top_marker_decibels = 0.0f,
            .marker_count = 0,
        };
        REQUIRE(params == wantParams);

        const std::vector<SpectrumPlot::Marker> want = {};
        REQUIRE(plot.GenerateDecibelScaleMarkers(params, 200) == want);
    }
}

TEST_CASE("SpectrumPlot::CalculateDecibelScaleParameters", "[spectrum_plot]")
{
    Settings settings;
    const AudioBuffer audioBuffer;
    const SpectrogramController controller(settings, audioBuffer);
    const SpectrumPlot plot(controller);

    // We'll just do a basic smoke test here. Detailed correctness is
    // verified in GenerateDecibelScaleMarkers() tests.

    SECTION("computes correct parameters for normal aperture")
    {
        settings.SetApertureMinDecibels(-60.0f);
        settings.SetApertureMaxDecibels(0.0f);
        const SpectrumPlot::DecibelScaleParameters want = {
            .aperture_min_decibels = -60.0f,
            .aperture_max_decibels = 0.0f,
            .pixels_per_decibel = 2.0f,
            .decibel_step = 10,
            .top_marker_decibels = 0.0f,
            .marker_count = 7,
        };
        REQUIRE(plot.CalculateDecibelScaleParameters(120) == want);
    }
}

TEST_CASE("SpectrumPlot::ComputeCrosshair", "[spectrum_plot]")
{
    Settings settings;
    const AudioBuffer audioBuffer;
    settings.SetFFTSettings(1024, FFTWindow::Type::Rectangular);
    const SpectrogramController controller(settings, audioBuffer, MockFFTProcessor::GetFactory());
    const SpectrumPlot plot(controller);

    SECTION("computes correct crosshair at center of widget")
    {
        settings.SetApertureMinDecibels(-100.0f);
        settings.SetApertureMaxDecibels(0.0f);

        const int kWidth = 512;
        const int kHeight = 200;
        const QPoint kMousePos(256, 100); // Center

        const auto markers = plot.ComputeCrosshair(kMousePos, kHeight, kWidth);
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
        settings.SetApertureMinDecibels(-100.0f);
        settings.SetApertureMaxDecibels(0.0f);

        const int kWidth = 512;
        const int kHeight = 200;
        const QPoint kMousePos(0, 0); // Top left

        const auto markers = plot.ComputeCrosshair(kMousePos, kHeight, kWidth);
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
        settings.SetApertureMinDecibels(-100.0f);
        settings.SetApertureMaxDecibels(0.0f);

        const int kWidth = 512;
        const int kHeight = 200;
        const QPoint kMousePos(511, 199); // Bottom right

        const auto markers = plot.ComputeCrosshair(kMousePos, kHeight, kWidth);
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
        settings.SetApertureMinDecibels(-60.0f);
        settings.SetApertureMaxDecibels(-20.0f);

        const int kWidth = 400;
        const int kHeight = 100;
        const QPoint kMousePos(200, 50); // Center

        const auto markers = plot.ComputeCrosshair(kMousePos, kHeight, kWidth);
        REQUIRE(markers.size() == 2);

        // At center of 100px height with -60 to -20 dB range:
        // Center should be -40 dB
        const auto& dbMarker = markers[1];
        CHECK(dbMarker.text == "-40 dB");
    }

    SECTION("handles inverted aperture")
    {
        settings.SetApertureMinDecibels(0.0f);
        settings.SetApertureMaxDecibels(-100.0f);

        const int kWidth = 512;
        const int kHeight = 200;
        const QPoint kMousePos(256, 100); // Center

        const auto markers = plot.ComputeCrosshair(kMousePos, kHeight, kWidth);
        REQUIRE(markers.size() == 2);

        // With inverted aperture, center should still compute correctly
        // normalized_y = 1 - (100/200) = 0.5
        // dB = 0 + (0.5 * -100) = -50 dB
        const auto& dbMarker = markers[1];
        CHECK(dbMarker.text == "-50 dB");
    }

    SECTION("computes correct frequency for different FFT sizes")
    {
        settings.SetFFTSettings(512, FFTWindow::Type::Rectangular);

        const int kWidth = 256;
        const int kHeight = 100;
        const QPoint kMousePos(128, 50); // Center

        const auto markers = plot.ComputeCrosshair(kMousePos, kHeight, kWidth);
        REQUIRE(markers.size() == 2);

        // Hz calculation: mouseX * HzPerBin = 128 * (44100/512) ≈ 11025 Hz
        const auto& freqMarker = markers[0];
        CHECK(freqMarker.text == "11025 Hz");
        // The exact value depends on sample rate from settings
    }
}