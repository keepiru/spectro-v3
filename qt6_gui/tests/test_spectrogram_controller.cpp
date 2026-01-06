#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "models/settings.h"
#include <audio_types.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <fft_window.h>
#include <ifft_processor.h>
#include <memory>
#include <mock_fft_processor.h>
#include <stdexcept>
#include <vector>

namespace {
using namespace Catch::Matchers;
}

TEST_CASE("SpectrogramController constructor", "[spectrogram_controller]")
{
    const Settings settings;
    const AudioBuffer audioBuffer;
    const SpectrogramController controller(settings, audioBuffer);
}

TEST_CASE("SpectrogramController::SetFFTSettings", "[spectrogram_controller]")
{
    std::vector<FFTSize> procCalls;
    const IFFTProcessorFactory procSpy = [&procCalls](FFTSize size) {
        procCalls.emplace_back(size);
        return std::make_unique<MockFFTProcessor>(size);
    };

    std::vector<std::pair<FFTSize, FFTWindow::Type>> windowCalls;
    const FFTWindowFactory windowSpy = [&windowCalls](FFTSize size, FFTWindow::Type type) {
        windowCalls.emplace_back(size, type);
        return std::make_unique<FFTWindow>(size, type);
    };

    Settings settings;
    const AudioBuffer audioBuffer;
    const SpectrogramController controller(settings, audioBuffer, procSpy, windowSpy);

    // Constructor calls with defaults
    REQUIRE(procCalls == (std::vector<FFTSize>{ SpectrogramController::KDefaultFftSize,
                                                SpectrogramController::KDefaultFftSize }));
    REQUIRE(windowCalls == (std::vector<std::pair<FFTSize, FFTWindow::Type>>{
                             std::make_pair(SpectrogramController::KDefaultFftSize,
                                            SpectrogramController::KDefaultWindowType),
                             std::make_pair(SpectrogramController::KDefaultFftSize,
                                            SpectrogramController::KDefaultWindowType),
                           }));

    settings.SetFFTSettings(1024, FFTWindow::Type::Rectangular);

    // SetFFTSettings calls again with new settings
    REQUIRE(procCalls == (std::vector<FFTSize>{ SpectrogramController::KDefaultFftSize,
                                                SpectrogramController::KDefaultFftSize,
                                                1024,
                                                1024 }));
    REQUIRE(windowCalls == (std::vector<std::pair<FFTSize, FFTWindow::Type>>{
                             std::make_pair(SpectrogramController::KDefaultFftSize,
                                            SpectrogramController::KDefaultWindowType),
                             std::make_pair(SpectrogramController::KDefaultFftSize,
                                            SpectrogramController::KDefaultWindowType),
                             std::make_pair(1024, FFTWindow::Type::Rectangular),
                             std::make_pair(1024, FFTWindow::Type::Rectangular) }));
}

namespace {
std::unique_ptr<SpectrogramController>
CreateControllerWithMockFFT(Settings& aSettings, AudioBuffer& aBuffer)
{

    aSettings.SetFFTSettings(8, FFTWindow::Type::Rectangular);
    auto controller = std::make_unique<SpectrogramController>(
      aSettings, aBuffer, MockFFTProcessor::GetFactory(), nullptr);
    aSettings.SetWindowScale(1);

    return controller;
}
} // namespace

TEST_CASE("SpectrogramController::GetRows single window computation", "[spectrogram_controller]")
{
    AudioBuffer buffer;
    buffer.Reset(1, 44100);
    buffer.AddSamples(
      { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 });
    Settings settings;
    auto controller = CreateControllerWithMockFFT(settings, buffer);

    const std::vector<std::vector<float>> kWant = {
        { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },
    };
    REQUIRE(controller->GetRows(0, 0, 1) == kWant);
    REQUIRE(controller->GetRow(0, 0) == kWant[0]);
    REQUIRE(controller->ComputeFFT(0, FrameIndex(0)) == kWant[0]);
}

TEST_CASE("SpectrogramController::GetRows multiple non-overlapping windows",
          "[spectrogram_controller]")
{
    AudioBuffer buffer;
    buffer.Reset(1, 44100);
    buffer.AddSamples(
      { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 });
    Settings settings;
    auto controller = CreateControllerWithMockFFT(settings, buffer);
    const std::vector<std::vector<float>> kWant = {
        { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },
        { 9.0f, 10.0f, 11.0f, 12.0f, 13.0f },
        { 17.0f, 18.0f, 19.0f, 20.0f, 21.0f },
    };
    const auto kGot = controller->GetRows(0, 0, 3);
    REQUIRE(kGot == kWant);
}

TEST_CASE("SpectrogramController::GetRows 50% overlap", "[spectrogram_controller]")
{
    Settings settings;
    AudioBuffer buffer;
    buffer.Reset(1, 44100);
    buffer.AddSamples(
      { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 });
    auto controller = CreateControllerWithMockFFT(settings, buffer);
    settings.SetWindowScale(2); // 50% overlap

    const std::vector<std::vector<float>> kWant = {
        { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },      { 5.0f, 6.0f, 7.0f, 8.0f, 9.0f },
        { 9.0f, 10.0f, 11.0f, 12.0f, 13.0f },  { 13.0f, 14.0f, 15.0f, 16.0f, 17.0f },
        { 17.0f, 18.0f, 19.0f, 20.0f, 21.0f },
    };
    const auto kGot = controller->GetRows(0, 0, 5);
    REQUIRE(kGot == kWant);
}

TEST_CASE("SpectrogramController::GetRows 75% overlap", "[spectrogram_controller]")
{
    Settings settings;
    AudioBuffer buffer;
    buffer.Reset(1, 44100);
    buffer.AddSamples({ 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13,
                        14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26 });
    auto controller = CreateControllerWithMockFFT(settings, buffer);
    settings.SetWindowScale(4); // 75% overlap

    const std::vector<std::vector<float>> kWant = {
        { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },      { 3.0f, 4.0f, 5.0f, 6.0f, 7.0f },
        { 5.0f, 6.0f, 7.0f, 8.0f, 9.0f },      { 7.0f, 8.0f, 9.0f, 10.0f, 11.0f },
        { 9.0f, 10.0f, 11.0f, 12.0f, 13.0f },  { 11.0f, 12.0f, 13.0f, 14.0f, 15.0f },
        { 13.0f, 14.0f, 15.0f, 16.0f, 17.0f }, { 15.0f, 16.0f, 17.0f, 18.0f, 19.0f },
        { 17.0f, 18.0f, 19.0f, 20.0f, 21.0f }, { 19.0f, 20.0f, 21.0f, 22.0f, 23.0f },
    };
    const auto kGot = controller->GetRows(0, 0, 10);
    REQUIRE(kGot == kWant);
}

TEST_CASE("SpectrogramController::GetRows with start sample beyond buffer end returns zeroed rows",
          "[spectrogram_controller]")
{
    Settings settings;
    AudioBuffer buffer;
    buffer.Reset(1, 44100);
    buffer.AddSamples({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });
    auto controller = CreateControllerWithMockFFT(settings, buffer);

    const std::vector<std::vector<float>> kWant = {
        { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    };

    REQUIRE(controller->GetRows(0, 0, 2) == kWant);

    // Do the same thing with GetRow
    REQUIRE(controller->GetRow(0, 0) == kWant[0]);
    REQUIRE(controller->GetRow(0, 8) == kWant[1]);

    // And with ComputeFFT
    REQUIRE(controller->ComputeFFT(0, FrameIndex(0)) == kWant[0]);
    REQUIRE_THROWS_AS((void)controller->ComputeFFT(0, FrameIndex(8)), std::out_of_range);
}

TEST_CASE("SpectrogramController::GetRows with negative start sample returns zeroed rows",
          "[spectrogram_controller]")
{
    Settings settings;
    AudioBuffer buffer;
    buffer.Reset(1, 44100);
    buffer.AddSamples({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });
    auto controller = CreateControllerWithMockFFT(settings, buffer);
    const std::vector<std::vector<float>> kWant = {
        { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
        { 7.0f, 8.0f, 9.0f, 10.0f, 11.0f },
    };
    REQUIRE(controller->GetRows(0, -2, 2) == kWant);

    // Do the same thing with GetRow
    REQUIRE(controller->GetRow(0, -2) == kWant[0]);
    REQUIRE(controller->GetRow(0, 6) == kWant[1]);

    // ComputeFFT takes FrameIndex (unsigned), so negative values cannot be passed
    // The validation happens in GetRow which calls ToFrameIndex before ComputeFFT
    REQUIRE(controller->ComputeFFT(0, FrameIndex(6)) == kWant[1]);
}

TEST_CASE("SpectrogramController::GetRows with Hann window integration", "[spectrogram_controller]")
{
    Settings settings;
    AudioBuffer buffer;
    buffer.Reset(1, 44100);
    buffer.AddSamples({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 });
    auto controller = CreateControllerWithMockFFT(settings, buffer);
    settings.SetFFTSettings(8, FFTWindow::Type::Hann);

    // Hann window attenuates edges, so we'll see lower magnitudes at the
    // edges.  Keep in mind our MockFFTProcessor just returns the input
    // samples as magnitudes so the only transformation is from the
    // windowing.
    const std::vector<std::vector<float>> kWant = {
        { 0.0f, 0.188255101f, 0.611260474f, 0.950484395f, 0.950484395f },
        { 0.0f, 0.188255101f, 0.611260474f, 0.950484395f, 0.950484395f }
    };

    REQUIRE(controller->GetRows(0, 0, 2) == kWant);
    REQUIRE(controller->GetRow(0, 0) == kWant[0]);
    REQUIRE(controller->ComputeFFT(0, FrameIndex(0)) == kWant[0]);
}

TEST_CASE("SpectrogramController::GetRows throws on invalid channel", "[spectrogram_controller]")
{
    const Settings settings;
    AudioBuffer buffer;
    buffer.Reset(1, 44100);
    const SpectrogramController controller(settings, buffer);

    REQUIRE_THROWS_AS((void)controller.GetRows(1, 0, 1), std::out_of_range);
    REQUIRE_THROWS_AS((void)controller.GetRow(1, 0), std::out_of_range);
    REQUIRE_THROWS_AS((void)controller.ComputeFFT(1, FrameIndex(0)), std::out_of_range);
}

TEST_CASE("SpectrogramController::GetAvailableSampleCount", "[spectrogram_controller]")
{
    const Settings settings;
    AudioBuffer buffer;
    const SpectrogramController controller(settings, buffer);

    buffer.AddSamples({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 });
    const auto kExpectedSampleCount = 5; // 10 samples / 2 channels
    REQUIRE(controller.GetAvailableFrameCount() == FrameCount(kExpectedSampleCount));
}

TEST_CASE("SpectrogramController::GetChannelCount", "[spectrogram_controller]")
{
    const Settings settings;
    AudioBuffer buffer;
    buffer.Reset(3, 44100);
    const SpectrogramController controller(settings, buffer);
    REQUIRE(controller.GetChannelCount() == 3);
}

TEST_CASE("SpectrogramController::CalculateTopOfWindow", "[spectrogram_controller]")
{
    Settings settings;
    const AudioBuffer audioBuffer;
    SpectrogramController controller(settings, audioBuffer);

    settings.SetFFTSettings(8, FFTWindow::Type::Rectangular);

    auto check = [&](int64_t index, size_t scale, int64_t want) {
        settings.SetWindowScale(scale);

        const int64_t have = controller.CalculateTopOfWindow(index);
        INFO(std::format("CalculateTopOfWindow: sample={} scale={} => topSample={} (want {})",
                         index,
                         scale,
                         have,
                         want));
        REQUIRE(have == want);
    };

    check(6, 1, -8); //[-8 -7 -6 -5 -4 -3 -2 -1] 0  1  2  3  4  5
    check(6, 2, -4); // -8 -7 -6 -5[-4 -3 -2 -1  0  1  2  3] 4  5
    check(6, 4, -2); // -8 -7 -6 -5 -4 -3[-2 -1  0  1  2  3  4  5]
    check(6, 8, -2); // -8 -7 -6 -5 -4 -3[-2 -1  0  1  2  3  4  5]

    check(7, 1, -8); //[-8 -7 -6 -5 -4 -3 -2 -1] 0  1  2  3  4  5  6
    check(7, 2, -4); // -8 -7 -6 -5[-4 -3 -2 -1  0  1  2  3] 4  5  6
    check(7, 4, -2); // -8 -7 -6 -5 -4 -3[-2 -1  0  1  2  3  4  5] 6
    check(7, 8, -1); // -8 -7 -6 -5 -4 -3 -2[-1  0  1  2  3  4  5  6]

    check(8, 1, 0); // -8 -7 -6 -5 -4 -3 -2 -1 [0  1  2  3  4  5  6  7]
    check(8, 2, 0); // -8 -7 -6 -5 -4 -3 -2 -1 [0  1  2  3  4  5  6  7]
    check(8, 4, 0); // -8 -7 -6 -5 -4 -3 -2 -1 [0  1  2  3  4  5  6  7]
    check(8, 8, 0); // -8 -7 -6 -5 -4 -3 -2 -1 [0  1  2  3  4  5  6  7]

    check(12, 1, 0); // [0  1  2  3  4  5  6  7] 8  9  10  11
    check(12, 2, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11]
    check(12, 4, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11]
    check(12, 8, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11]

    check(13, 1, 0); // [0  1  2  3  4  5  6  7] 8  9  10  11  12
    check(13, 2, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11] 12
    check(13, 4, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11] 12
    check(13, 8, 5); //  0  1  2  3  4 [5  6  7  8  9  10  11  12]

    check(14, 1, 0); // [0  1  2  3  4  5  6  7] 8  9  10  11  12  13
    check(14, 2, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11] 12  13
    check(14, 4, 6); //  0  1  2  3  4  5 [6  7  8  9  10  11  12  13]
    check(14, 8, 6); //  0  1  2  3  4  5 [6  7  8  9  10  11  12  13]

    check(15, 1, 0); // [0  1  2  3  4  5  6  7] 8  9  10  11  12  13  14
    check(15, 2, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11] 12  13  14
    check(15, 4, 6); //  0  1  2  3  4  5 [6  7  8  9  10  11  12  13] 14
    check(15, 8, 7); //  0  1  2  3  4  5  6 [7  8  9  10  11  12  13  14]

    check(16, 1, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15]
    check(16, 2, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15]
    check(16, 4, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15]
    check(16, 8, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15]

    check(17, 1, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15] 16
    check(17, 2, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15] 16
    check(17, 4, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15] 16
    check(17, 8, 9); //  0  1  2  3  4  5  6  7  8 [9  10  11  12  13  14  15  16]

    check(18, 1, 8);  //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15] 16  17
    check(18, 2, 8);  //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15] 16  17
    check(18, 4, 10); //  0  1  2  3  4  5  6  7  8  9 [10  11  12  13  14  15  16  17]
    check(18, 8, 10); //  0  1  2  3  4  5  6  7  8  9 [10  11  12  13  14  15  16  17]
}

TEST_CASE("SpectrogramController::RoundToStride", "[spectrogram_controller]")
{
    Settings settings;
    const AudioBuffer audioBuffer;
    SpectrogramController controller(settings, audioBuffer);

    settings.SetFFTSettings(8, FFTWindow::Type::Rectangular);

    auto check = [&](size_t stride, int64_t sample, int64_t want) {
        if (settings.GetFFTSize() % stride != 0) {
            throw std::invalid_argument("Stride must divide FFT size evenly");
        }
        settings.SetWindowScale(settings.GetFFTSize() / stride);

        const int64_t have = controller.RoundToStride(sample);
        INFO(std::format(
          "SpectrogramController::RoundToStride: stride={}, sample={}, got={} (want {})",
          stride,
          sample,
          have,
          want));
        REQUIRE(have == want);
    };

    check(1, -2, -2);
    check(1, -1, -1);
    check(1, 0, 0);
    check(1, 1, 1);
    check(1, 2, 2);

    check(2, -3, -4);
    check(2, -2, -2);
    check(2, -1, -2);
    check(2, 0, 0);
    check(2, 1, 0);
    check(2, 2, 2);

    check(4, -5, -8);
    check(4, -4, -4);
    check(4, -3, -4);
    check(4, -2, -4);
    check(4, -1, -4);
    check(4, 0, 0);
    check(4, 1, 0);
    check(4, 2, 0);
    check(4, 3, 0);
    check(4, 4, 4);
    check(4, 5, 4);

    check(8, -10, -16);
    check(8, -9, -16);
    check(8, -8, -8);
    check(8, -1, -8);
    check(8, 0, 0);
    check(8, 7, 0);
    check(8, 8, 8);
    check(8, 15, 8);
    check(8, 16, 16);
    check(8, 17, 16);
}

TEST_CASE("SpectrogramController::GetHzPerBin", "[spectrogram_controller]")
{
    Settings settings;
    AudioBuffer buffer;
    buffer.Reset(2, 48000); // 48 kHz sample rate
    const SpectrogramController controller(settings, buffer);

    settings.SetFFTSettings(1024, FFTWindow::Type::Rectangular);
    CHECK_THAT(controller.GetHzPerBin(), WithinAbs(46.875f, 0.0001f)); // 48000 / 1024

    settings.SetFFTSettings(2048, FFTWindow::Type::Rectangular);
    CHECK_THAT(controller.GetHzPerBin(), WithinAbs(23.4375f, 0.0001f)); // 48000 / 2048

    buffer.Reset(1, 44100); // 44.1 kHz sample rate
    settings.SetFFTSettings(1024, FFTWindow::Type::Rectangular);
    CHECK_THAT(controller.GetHzPerBin(), WithinAbs(43.06640625f, 0.0001f)); // 44100 / 1024
}