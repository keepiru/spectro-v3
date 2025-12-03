#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <numbers>
#include <fft_processor.h>
#include <cmath>

TEST_CASE("Constructor succeeds", "[fft]")
{
    const uint32_t transform_size = 1024;
    FFTProcessor processor(transform_size);

    REQUIRE(processor.getTransformSize() == transform_size);

    SECTION("Power of 2 transforms are accepted", "[fft]")
    {
        REQUIRE_NOTHROW(FFTProcessor(256));
        REQUIRE_NOTHROW(FFTProcessor(512));
        REQUIRE_NOTHROW(FFTProcessor(2048));
    }

    SECTION("Non-power of 2 transforms are rejected", "[fft]")
    {
        REQUIRE_THROWS_AS(FFTProcessor(300), std::invalid_argument);
        REQUIRE_THROWS_AS(FFTProcessor(1000), std::invalid_argument);
        REQUIRE_THROWS_AS(FFTProcessor(1500), std::invalid_argument);
    }
}

TEST_CASE("FFTProcessor move/copy semantics", "[fft]")
{
    const uint32_t transform_size = 512;
    FFTProcessor processor1(transform_size);

    FFTProcessor processor2(std::move(processor1));
    REQUIRE(processor2.getTransformSize() == transform_size);

    FFTProcessor processor3(256);
    processor3 = std::move(processor2);
    REQUIRE(processor3.getTransformSize() == transform_size);

    static_assert(!std::is_copy_constructible_v<FFTProcessor>);
    static_assert(!std::is_copy_assignable_v<FFTProcessor>);
    static_assert(std::is_move_constructible_v<FFTProcessor>);
    static_assert(std::is_move_assignable_v<FFTProcessor>);
}

TEST_CASE("FFTProcessor#compute_complex", "[fft]")
{
    const uint32_t transform_size = 8; // Small for testing
    FFTProcessor processor(transform_size);

    SECTION("Throws on input size mismatch", "[fft]")
    {
        std::vector<float> samples(transform_size - 1, 0.0f); // Incorrect size
        REQUIRE_THROWS_AS(processor.compute_complex(samples), std::invalid_argument);
    }

    SECTION("Compute complex FFT output", "[fft]")
    {
        std::vector<float> samples = { 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f };
        auto fft_output = processor.compute_complex(samples);

        REQUIRE(fft_output.size() == transform_size / 2 + 1);
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
    const uint32_t transform_size = 8; // Small for testing
    FFTProcessor processor(transform_size);

    SECTION("Throws on input size mismatch", "[fft]")
    {
        std::vector<float> samples(transform_size - 1, 0.0f); // Incorrect size
        REQUIRE_THROWS_AS(processor.compute_magnitudes(samples), std::invalid_argument);
    }

    SECTION("Computes DC component for constant signal", "[fft]")
    {
        std::vector<float> samples(transform_size, 1.0f);
        auto spectrum = processor.compute_magnitudes(samples);

        REQUIRE(spectrum.size() == transform_size / 2 + 1);
        REQUIRE_THAT(spectrum[0], Catch::Matchers::WithinRel(8.0));
    }

    SECTION("Computes peak frequency of sine wave", "[fft]")
    {
        const float frequency = 1.0f; // 1 cycle over 8 samples
        std::vector<float> samples(transform_size);
        for (size_t i = 0; i < transform_size; ++i) {
            samples[i] =
              sinf(2.0f * std::numbers::pi_v<float> * frequency * (i / static_cast<float>(transform_size)));
        }

        auto spectrum = processor.compute_magnitudes(samples);

        REQUIRE(spectrum.size() == transform_size / 2 + 1);
        REQUIRE_THAT(spectrum[0], Catch::Matchers::WithinAbs(0.0, 0.00001)); // No DC component
        REQUIRE_THAT(spectrum[1], Catch::Matchers::WithinAbs(4.0, 0.00001)); // Peak at bin 1
        REQUIRE_THAT(spectrum[2], Catch::Matchers::WithinAbs(0.0, 0.00001)); // No bin 2 component
        REQUIRE_THAT(spectrum[3], Catch::Matchers::WithinAbs(0.0, 0.00001)); // No bin 3 component
        REQUIRE_THAT(spectrum[4], Catch::Matchers::WithinAbs(0.0, 0.00001)); // No Nyquist component
    }
}
