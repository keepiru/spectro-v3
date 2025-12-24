#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "views/spectrogram_view.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("SpectrogramView constructor", "[spectrogram_view]")
{
    const Settings settings;
    const AudioBuffer audioBuffer;
    SpectrogramController controller(settings, audioBuffer);
    const SpectrogramView view(controller);

    REQUIRE(view.minimumWidth() > 0);
    REQUIRE(view.minimumHeight() > 0);
}

TEST_CASE("SpectrogramView is widget", "[spectrogram_view]")
{
    const Settings settings;
    const AudioBuffer audioBuffer;
    SpectrogramController controller(settings, audioBuffer);
    SpectrogramView view(controller);
    REQUIRE(qobject_cast<QWidget*>(&view) != nullptr);
}
