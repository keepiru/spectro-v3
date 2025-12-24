#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "views/spectrum_plot.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("SpectrumPlot constructor", "[spectrum_plot]")
{
    const Settings settings;
    const AudioBuffer audioBuffer;
    SpectrogramController controller(settings, audioBuffer);
    const SpectrumPlot plot(controller);
    REQUIRE(plot.minimumWidth() > 0);
    REQUIRE(plot.minimumHeight() > 0);
}

TEST_CASE("SpectrumPlot is widget", "[spectrum_plot]")
{
    const Settings settings;
    const AudioBuffer audioBuffer;
    SpectrogramController controller(settings, audioBuffer);
    SpectrumPlot plot(controller);
    REQUIRE(qobject_cast<QWidget*>(&plot) != nullptr);
}
