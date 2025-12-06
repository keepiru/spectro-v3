#include "mock_fft_processor.h"
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <fft_window.h>
#include <sample_buffer.h>
#include <stdexcept>
#include <stft_processor.h>
#include <vector>

TEST_CASE("STFTProcessor constructor validation", "[stft]")
{
    const uint32_t kTransformSize = 512;

    MockFFTProcessor fftProcessor(kTransformSize);
    const SampleBuffer buffer(44100);

    SECTION("Constructor succeeds when window size matches transform size")
    {
        FFTWindow window(kTransformSize, FFTWindow::Type::kHann);
        REQUIRE_NOTHROW(STFTProcessor(fftProcessor, window, buffer));
    }

    SECTION("Constructor throws when window size does not match transform size")
    {
        FFTWindow window(kTransformSize / 2, FFTWindow::Type::kHann);
        REQUIRE_THROWS_AS(STFTProcessor(fftProcessor, window, buffer), std::invalid_argument);
    }

    SECTION("Constructor throws when window size is larger than transform size")
    {
        FFTWindow window(static_cast<size_t>(kTransformSize * 2), FFTWindow::Type::kHann);
        REQUIRE_THROWS_AS(STFTProcessor(fftProcessor, window, buffer), std::invalid_argument);
    }
}

TEST_CASE("STFTProcessor ComputeSpectrogram validation", "[stft]")
{
    const uint32_t kTransformSize = 512;

    MockFFTProcessor fftProcessor(kTransformSize);
    FFTWindow window(kTransformSize, FFTWindow::Type::kRectangular);
    const SampleBuffer buffer(44100);
    STFTProcessor const kSTFT(fftProcessor, window, buffer);

    SECTION("Throws on zero window_stride")
    {
        REQUIRE_THROWS_AS(kSTFT.ComputeSpectrogram(0, 0, 10), std::invalid_argument);
    }
}

TEST_CASE("STFTProcessor basic spectrogram computation", "[stft]")
{
    const uint32_t kTransformSize = 8;

    MockFFTProcessor fftProcessor(kTransformSize);
    FFTWindow window(kTransformSize, FFTWindow::Type::kRectangular);
    SampleBuffer buffer(44100);

    buffer.AddSamples(
      { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 });

    STFTProcessor const kSTFT(fftProcessor, window, buffer);

    SECTION("Single window computation")
    {
        const std::vector<std::vector<float>> kWant = {
            { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f },
        };
        const auto kGot = kSTFT.ComputeSpectrogram(0, kTransformSize, 1);

        REQUIRE(kGot == kWant);
    }

    SECTION("Multiple non-overlapping windows")
    {
        const size_t kWindowCount = 3;
        const std::vector<std::vector<float>> kWant = {
            { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f },
            { 8.0f, 9.0f, 10.0f, 11.0f, 12.0f },
            { 16.0f, 17.0f, 18.0f, 19.0f, 20.0f },
        };
        const auto kGot = kSTFT.ComputeSpectrogram(0, kTransformSize, kWindowCount);

        REQUIRE(kGot == kWant);
    }

    SECTION("Overlapping windows (50% overlap)")
    {
        const size_t kWindowStride = kTransformSize / 2; // 50% overlap
        const size_t kWindowCount = 5;
        const std::vector<std::vector<float>> kWant = {
            { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f },      { 4.0f, 5.0f, 6.0f, 7.0f, 8.0f },
            { 8.0f, 9.0f, 10.0f, 11.0f, 12.0f },   { 12.0f, 13.0f, 14.0f, 15.0f, 16.0f },
            { 16.0f, 17.0f, 18.0f, 19.0f, 20.0f },
        };
        const auto kGot = kSTFT.ComputeSpectrogram(0, kWindowStride, kWindowCount);

        REQUIRE(kGot == kWant);
    }

    SECTION("High overlap (75% overlap)")
    {
        const size_t kWindowStride = kTransformSize / 4; // 75% overlap
        const size_t kWindowCount = 10;
        const std::vector<std::vector<float>> kWant = {
            { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f },      { 2.0f, 3.0f, 4.0f, 5.0f, 6.0f },
            { 4.0f, 5.0f, 6.0f, 7.0f, 8.0f },      { 6.0f, 7.0f, 8.0f, 9.0f, 10.0f },
            { 8.0f, 9.0f, 10.0f, 11.0f, 12.0f },   { 10.0f, 11.0f, 12.0f, 13.0f, 14.0f },
            { 12.0f, 13.0f, 14.0f, 15.0f, 16.0f }, { 14.0f, 15.0f, 16.0f, 17.0f, 18.0f },
            { 16.0f, 17.0f, 18.0f, 19.0f, 20.0f }, { 18.0f, 19.0f, 20.0f, 21.0f, 22.0f },
        };
        const auto kGot = kSTFT.ComputeSpectrogram(0, kWindowStride, kWindowCount);
        REQUIRE(kGot == kWant);
    }
}

TEST_CASE("STFTProcessor handles negative start_frame", "[stft]")
{
    const uint32_t kTransformSize = 8;

    MockFFTProcessor fftProcessor(kTransformSize);
    FFTWindow window(kTransformSize, FFTWindow::Type::kRectangular);
    SampleBuffer buffer(44100);

    buffer.AddSamples({ 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f });

    STFTProcessor const kSTFT(fftProcessor, window, buffer);

    SECTION("Negative start_frame with zero-padding from SampleBuffer")
    {
        const std::vector<std::vector<float>> kWant = {
            { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f },
            { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },
            { 5.0f, 6.0f, 7.0f, 8.0f, 0.0f },
        };
        const auto kGot = kSTFT.ComputeSpectrogram(-4, 4, 3);

        REQUIRE(kGot == kWant);
    }
}

TEST_CASE("STFTProcessor handles start_frame beyond buffer end", "[stft]")
{
    const uint32_t transformSize = 8;

    MockFFTProcessor fftProcessor(transformSize);
    FFTWindow window(transformSize, FFTWindow::Type::kRectangular);
    SampleBuffer buffer(44100);

    buffer.AddSamples({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });

    STFTProcessor const kSTFT(fftProcessor, window, buffer);

    SECTION("Windows extending past buffer end get zero-padded")
    {
        const std::vector<std::vector<float>> kWant = {
            { 12.0f, 13.0f, 14.0f, 15.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
        };
        const auto kGot = kSTFT.ComputeSpectrogram(12, transformSize, 2);

        REQUIRE(kGot == kWant);
    }
}

TEST_CASE("STFTProcessor with kHann window integration", "[stft]")
{
    const uint32_t kTransformSize = 8;

    MockFFTProcessor fftProcessor(8);
    FFTWindow window(kTransformSize, FFTWindow::Type::kHann);
    SampleBuffer buffer(44100);

    buffer.AddSamples({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 });

    STFTProcessor const kSTFT(fftProcessor, window, buffer);

    SECTION("kHann window is applied before FFT")
    {
        // Hann window attenuates edges, so we'll see lower magnitudes at the
        // edges.  Keep in mind our MockFFTProcessor just returns the input
        // samples as magnitudes so the only transformation is from the
        // windowing.
        const std::vector<std::vector<float>> kWant = {
            { 0.0f, 0.188255101f, 0.611260474f, 0.950484395f, 0.950484395f },
            { 0.0f, 0.188255101f, 0.611260474f, 0.950484395f, 0.950484395f }
        };
        const auto kGot = kSTFT.ComputeSpectrogram(0, 4, 2);
        REQUIRE(kGot == kWant);
    }
}
