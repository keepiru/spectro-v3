#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <fft_processor.h>
#include <math.h>

TEST_CASE("Constructor succeeds", "[fft]")
{
    const uint32_t bins = 1024;
    FFTProcessor processor(bins);

    REQUIRE(processor.getNumBins() == bins);

    SECTION("Power of 2 bins are accepted", "[fft]")
    {
        REQUIRE_NOTHROW(FFTProcessor(256));
        REQUIRE_NOTHROW(FFTProcessor(512));
        REQUIRE_NOTHROW(FFTProcessor(2048));
    }

    SECTION("Non-power of 2 bins are rejected", "[fft]")
    {
        REQUIRE_THROWS_AS(FFTProcessor(300), std::invalid_argument);
        REQUIRE_THROWS_AS(FFTProcessor(1000), std::invalid_argument);
        REQUIRE_THROWS_AS(FFTProcessor(1500), std::invalid_argument);
    }
}

TEST_CASE("FFTProcessor move/copy semantics", "[fft]")
{
    const uint32_t bins = 512;
    FFTProcessor processor1(bins);

    FFTProcessor processor2(std::move(processor1));
    REQUIRE(processor2.getNumBins() == bins);

    FFTProcessor processor3(256);
    processor3 = std::move(processor2);
    REQUIRE(processor3.getNumBins() == bins);

    static_assert(!std::is_copy_constructible_v<FFTProcessor>);
    static_assert(!std::is_copy_assignable_v<FFTProcessor>);
    static_assert(std::is_move_constructible_v<FFTProcessor>);
    static_assert(std::is_move_assignable_v<FFTProcessor>);
}

TEST_CASE("FFTProcessor#compute_complex", "[fft]")
{
    const uint32_t bins = 8; // Small for testing
    FFTProcessor processor(bins);

    SECTION("Throws on input size mismatch", "[fft]")
    {
        std::vector<float> samples(bins - 1, 0.0f); // Incorrect size
        REQUIRE_THROWS_AS(processor.compute_complex(samples), std::invalid_argument);
    }

    SECTION("Compute complex FFT output", "[fft]")
    {
        std::vector<float> samples = { 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f };
        auto fft_output = processor.compute_complex(samples);

        REQUIRE(fft_output.size() == bins / 2 + 1);
        REQUIRE_THAT(fft_output[0][0], Catch::Matchers::WithinRel(0.0));  // DC real part
        REQUIRE_THAT(fft_output[0][1], Catch::Matchers::WithinRel(0.0));  // DC imag part
        REQUIRE_THAT(fft_output[1][0], Catch::Matchers::WithinRel(0.0));  // Bin 1 real part
        REQUIRE_THAT(fft_output[1][1], Catch::Matchers::WithinRel(0.0));  // Bin 1 imag part
        REQUIRE_THAT(fft_output[2][0], Catch::Matchers::WithinRel(0.0));  // Bin 2 real part
        REQUIRE_THAT(fft_output[2][1], Catch::Matchers::WithinRel(-4.0)); // Bin 2 imag part
        REQUIRE_THAT(fft_output[3][0], Catch::Matchers::WithinRel(0.0));  // Bin 3 real part
        REQUIRE_THAT(fft_output[3][1], Catch::Matchers::WithinRel(0.0));  // Bin 3 imag part
        REQUIRE_THAT(fft_output[4][0], Catch::Matchers::WithinRel(0.0));  // Nyquist real part
        REQUIRE_THAT(fft_output[4][1], Catch::Matchers::WithinRel(0.0));  // Nyquist imag part
    }
}

TEST_CASE("FFTProcessor#compute_magnitudes", "[fft]")
{
    const uint32_t bins = 8; // Small for testing
    FFTProcessor processor(bins);

    SECTION("Throws on input size mismatch", "[fft]")
    {
        std::vector<float> samples(bins - 1, 0.0f); // Incorrect size
        REQUIRE_THROWS_AS(processor.compute_magnitudes(samples), std::invalid_argument);
    }

    SECTION("Computes DC component for constant signal", "[fft]")
    {
        std::vector<float> samples(bins, 1.0f);
        auto spectrum = processor.compute_magnitudes(samples);

        REQUIRE(spectrum.size() == bins / 2 + 1);
        REQUIRE_THAT(spectrum[0], Catch::Matchers::WithinRel(8.0));
    }

    SECTION("Computes peak frequency of sine wave", "[fft]")
    {
        const float frequency = 1.0f; // 1 cycle over 8 samples
        std::vector<float> samples(bins);
        for (size_t i = 0; i < bins; ++i) {
            samples[i] = sinf(2.0f * 3.14159265f * frequency * (i / static_cast<float>(bins)));
        }

        auto spectrum = processor.compute_magnitudes(samples);

        REQUIRE(spectrum.size() == bins / 2 + 1);
        REQUIRE_THAT(spectrum[0], Catch::Matchers::WithinAbs(0.0, 0.00001)); // No DC component
        REQUIRE_THAT(spectrum[1], Catch::Matchers::WithinAbs(4.0, 0.00001)); // Peak at bin 1
        REQUIRE_THAT(spectrum[2], Catch::Matchers::WithinAbs(0.0, 0.00001)); // No bin 2 component
        REQUIRE_THAT(spectrum[3], Catch::Matchers::WithinAbs(0.0, 0.00001)); // No bin 3 component
        REQUIRE_THAT(spectrum[4], Catch::Matchers::WithinAbs(0.0, 0.00001)); // No Nyquist component
    }
}
