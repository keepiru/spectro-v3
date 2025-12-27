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