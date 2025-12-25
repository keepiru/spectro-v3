#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "views/spectrogram_view.h"
#include <catch2/catch_all.hpp>
#include <mock_fft_processor.h>
#include <vector>

using namespace Catch::Matchers;

namespace {

/**
 * @brief Convert a QImage to a string representation for easy comparison in tests.
 * @param image The QImage to convert.
 * @return A string representation of the image's pixel RGB values.
 */
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

// NOLINTNEXTLINE(readability-function-cognitive-complexity) macro expansion
TEST_CASE("SpectrogramView::GenerateSpectrogramImage", "[spectrogram_view]")
{
    Settings settings;

    AudioBuffer audioBuffer;
    SpectrogramController controller(
      settings, audioBuffer, MockFFTProcessor::GetFactory(), nullptr);
    SpectrogramView view(controller);

    SECTION("throws on invalid channel count")
    {
        // mock controller to return invalid channel count
        class MockController : public SpectrogramController
        {
          public:
            using SpectrogramController::SpectrogramController;
            size_t GetChannelCount() const override { return GENERATE(0, gkMaxChannels + 1); }
        };
        MockController mockController(settings, audioBuffer);
        SpectrogramView mockView(mockController);
        REQUIRE_THROWS_MATCHES(mockView.GenerateSpectrogramImage(256, 256),
                               std::runtime_error,
                               MessageMatches(ContainsSubstring("out of range")));
    }

    SECTION("generates image of correct size")
    {
        constexpr size_t width = 512;
        constexpr size_t height = 256;
        const QImage image = view.GenerateSpectrogramImage(width, height);
        CHECK(image.width() == width);
        CHECK(image.height() == height);
    }

    SECTION("generates correct image data based on audio buffer samples")
    {
        // Integration test pulling data through the full pipeline.

        // With this aperture and a linear colormap, the input samples 0..255 will
        // go through the mock FFT, become decibel ranges 0..255, and map directly
        // to RGB values.
        settings.SetApertureMinDecibels(0);
        settings.SetApertureMaxDecibels(255);

        // Fill audio buffer with known samples
        audioBuffer.Reset(1, 44100);
        audioBuffer.AddSamples({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 });

        settings.SetFFTSettings(8, FFTWindow::Type::Rectangular);
        settings.SetWindowScale(1); // no overlap

        const std::string kHave = QImageToString(view.GenerateSpectrogramImage(6, 4));
        const std::string kWant = "\n"
                                  "000101 000202 000303 000404 000505 000000 \n"
                                  "000909 000A0A 000B0B 000C0C 000D0D 000000 \n"
                                  "000000 000000 000000 000000 000000 000000 \n"
                                  "000000 000000 000000 000000 000000 000000 \n";

        REQUIRE(kHave == kWant);
    }
}