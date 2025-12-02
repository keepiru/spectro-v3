#include "mock_fft_processor.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <fft_window.h>
#include <sample_buffer.h>
#include <stft_processor.h>

TEST_CASE("STFTProcessor constructor validation", "[stft]")
{
    const uint32_t transform_size = 512;

    MockFFTProcessor fft_processor(transform_size);
    SampleBuffer buffer(44100);

    SECTION("Constructor succeeds when window size matches transform size")
    {
        FFTWindow window(transform_size, FFTWindow::Type::Hann);
        REQUIRE_NOTHROW(STFTProcessor(fft_processor, window, buffer));
    }

    SECTION("Constructor throws when window size does not match transform size")
    {
        FFTWindow window(transform_size / 2, FFTWindow::Type::Hann);
        REQUIRE_THROWS_AS(STFTProcessor(fft_processor, window, buffer), std::invalid_argument);
    }

    SECTION("Constructor throws when window size is larger than transform size")
    {
        FFTWindow window(transform_size * 2, FFTWindow::Type::Hann);
        REQUIRE_THROWS_AS(STFTProcessor(fft_processor, window, buffer), std::invalid_argument);
    }
}

TEST_CASE("STFTProcessor compute_spectrogram validation", "[stft]")
{
    const uint32_t transform_size = 512;

    MockFFTProcessor fft_processor(transform_size);
    FFTWindow window(transform_size, FFTWindow::Type::Rectangular);
    SampleBuffer buffer(44100);
    STFTProcessor stft(fft_processor, window, buffer);

    SECTION("Throws on zero window_stride")
    {
        REQUIRE_THROWS_AS(stft.compute_spectrogram(0, 0, 10), std::invalid_argument);
    }
}

TEST_CASE("STFTProcessor basic spectrogram computation", "[stft]")
{
    const uint32_t transform_size = 8;

    MockFFTProcessor fft_processor(transform_size);
    FFTWindow window(transform_size, FFTWindow::Type::Rectangular);
    SampleBuffer buffer(44100);

    buffer.add_samples(
      { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 });

    STFTProcessor stft(fft_processor, window, buffer);

    SECTION("Single window computation")
    {
        std::vector<std::vector<float>> want = {
            { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f },
        };
        auto got = stft.compute_spectrogram(0, transform_size, 1);

        REQUIRE(got == want);
    }

    SECTION("Multiple non-overlapping windows")
    {
        const size_t window_count = 3;
        std::vector<std::vector<float>> want = {
            { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f },
            { 8.0f, 9.0f, 10.0f, 11.0f, 12.0f },
            { 16.0f, 17.0f, 18.0f, 19.0f, 20.0f },
        };
        auto got = stft.compute_spectrogram(0, transform_size, window_count);

        REQUIRE(got == want);
    }

    SECTION("Overlapping windows (50% overlap)")
    {
        const size_t window_stride = transform_size / 2; // 50% overlap
        const size_t window_count = 5;
        std::vector<std::vector<float>> want = {
            { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f },      { 4.0f, 5.0f, 6.0f, 7.0f, 8.0f },
            { 8.0f, 9.0f, 10.0f, 11.0f, 12.0f },   { 12.0f, 13.0f, 14.0f, 15.0f, 16.0f },
            { 16.0f, 17.0f, 18.0f, 19.0f, 20.0f },
        };
        auto got = stft.compute_spectrogram(0, window_stride, window_count);

        REQUIRE(got == want);
    }

    SECTION("High overlap (75% overlap)")
    {
        const size_t window_stride = transform_size / 4; // 75% overlap
        const size_t window_count = 10;
        std::vector<std::vector<float>> want = {
            { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f },      { 2.0f, 3.0f, 4.0f, 5.0f, 6.0f },
            { 4.0f, 5.0f, 6.0f, 7.0f, 8.0f },      { 6.0f, 7.0f, 8.0f, 9.0f, 10.0f },
            { 8.0f, 9.0f, 10.0f, 11.0f, 12.0f },   { 10.0f, 11.0f, 12.0f, 13.0f, 14.0f },
            { 12.0f, 13.0f, 14.0f, 15.0f, 16.0f }, { 14.0f, 15.0f, 16.0f, 17.0f, 18.0f },
            { 16.0f, 17.0f, 18.0f, 19.0f, 20.0f }, { 18.0f, 19.0f, 20.0f, 21.0f, 22.0f },
        };
        auto got = stft.compute_spectrogram(0, window_stride, window_count);
        REQUIRE(got == want);
    }
}

TEST_CASE("STFTProcessor handles negative start_frame", "[stft]")
{
    const uint32_t transform_size = 8;

    MockFFTProcessor fft_processor(transform_size);
    FFTWindow window(transform_size, FFTWindow::Type::Rectangular);
    SampleBuffer buffer(44100);

    buffer.add_samples({ 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f });

    STFTProcessor stft(fft_processor, window, buffer);

    SECTION("Negative start_frame with zero-padding from SampleBuffer")
    {
        std::vector<std::vector<float>> want = {
            { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f },
            { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },
            { 5.0f, 6.0f, 7.0f, 8.0f, 0.0f },
        };
        auto got = stft.compute_spectrogram(-4, 4, 3);

        REQUIRE(got == want);
    }
}

TEST_CASE("STFTProcessor handles start_frame beyond buffer end", "[stft]")
{
    const uint32_t transform_size = 8;

    MockFFTProcessor fft_processor(transform_size);
    FFTWindow window(transform_size, FFTWindow::Type::Rectangular);
    SampleBuffer buffer(44100);

    buffer.add_samples({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });

    STFTProcessor stft(fft_processor, window, buffer);

    SECTION("Windows extending past buffer end get zero-padded")
    {
        std::vector<std::vector<float>> want = {
            { 12.0f, 13.0f, 14.0f, 15.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
        };
        auto got = stft.compute_spectrogram(12, transform_size, 2);

        REQUIRE(got == want);
    }
}

TEST_CASE("STFTProcessor with Hann window integration", "[stft]")
{
    const uint32_t transform_size = 8;

    MockFFTProcessor fft_processor(8);
    FFTWindow window(transform_size, FFTWindow::Type::Hann);
    SampleBuffer buffer(44100);

    buffer.add_samples({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 });

    STFTProcessor stft(fft_processor, window, buffer);

    SECTION("Hann window is applied before FFT")
    {
        // Hann window attenuates edges, so we'll see lower magnitudes at the
        // edges.  Keep in mind our MockFFTProcessor just returns the input
        // samples as magnitudes so the only transformation is from the
        // windowing.
        std::vector<std::vector<float>> want = {
            { 0.0f, 0.188255101f, 0.611260474f, 0.950484455f, 0.950484455f },
            { 0.0f, 0.188255101f, 0.611260474f, 0.950484455f, 0.950484455f },
        };
        auto result = stft.compute_spectrogram(0, 4, 2);
        REQUIRE(result == want);
    }
}
