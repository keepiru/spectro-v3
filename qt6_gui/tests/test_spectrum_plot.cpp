#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "views/spectrum_plot.h"
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

    auto fillBuffer = [&audioBuffer](size_t aChannelCount, size_t aSampleCount) {
        audioBuffer.Reset(aChannelCount, 44100);
        for (size_t i = 0; i < aSampleCount; ++i) {
            audioBuffer.AddSamples({ static_cast<float>(2 * i), static_cast<float>((2 * i) + 1) });
        }
    };

    SECTION("returns correct values for last available stride")
    {
        fillBuffer(2, 15);

        // Check our assumptions.  15 samples loaded:
        const int64_t kAvailableSampleCount = controller.GetAvailableSampleCount();
        CHECK(kAvailableSampleCount == 15);

        // With window size 8 and stride 4, top of window should start at sample 4
        const int64_t kTopSample = controller.CalculateTopOfWindow(kAvailableSampleCount);
        CHECK(kTopSample == 4);

        // Therefore, FFT window should cover samples 4-8:
        const std::vector<std::vector<float>> want = { { 8, 10, 12, 14, 16 },
                                                       { 9, 11, 13, 15, 17 } };
        CHECK(plot.GetDecibels(0) == want[0]);
        CHECK(plot.GetDecibels(1) == want[1]);
    }

    SECTION("returns zeroed vector if insufficient samples")
    {
        fillBuffer(2, 7); // Less than window size of 8
        const std::vector<float> want = { 0, 0, 0, 0, 0 };
        CHECK(plot.GetDecibels(0) == want);
        CHECK(plot.GetDecibels(1) == want);
    }

    SECTION("throws out_of_range for invalid channel")
    {
        fillBuffer(1, 20);
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

        const std::vector<SpectrumPlot::DecibelMarker> want = {
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

        const std::vector<SpectrumPlot::DecibelMarker> want = {
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

        const std::vector<SpectrumPlot::DecibelMarker> want = {
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

        const std::vector<SpectrumPlot::DecibelMarker> want = {
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

        const std::vector<SpectrumPlot::DecibelMarker> want = {};
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