#include "mock_fft_processor.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("MockFFTProcessor returns fixed values", "[MockFFTProcessor]")
{
    const uint32_t num_bins = 8;
    MockFFTProcessor mock_fft(num_bins);

    std::vector<float> samples = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f };

    SECTION("compute_magnitudes returns fixed values")
    {
        REQUIRE(mock_fft.compute_magnitudes(samples) ==
                std::vector<float>({ 0.0f, 1.0f, 2.0f, 3.0f, 4.0f }));
    }

    SECTION("compute_complex returns fixed complex values")
    {
        auto complex_output = mock_fft.compute_complex(samples);
        CAPTURE(complex_output);
        REQUIRE(complex_output.size() == samples.size() / 2 + 1);
        for (size_t i = 0; i < complex_output.size(); ++i) {
            REQUIRE(complex_output[i][0] == samples[i]); // Real part
            REQUIRE(complex_output[i][1] == samples[i]); // Imaginary part
        }
    }
}

TEST_CASE("MockFFTProcessor throws on size mismatch", "[MockFFTProcessor]")
{
    MockFFTProcessor mock_fft(8);
    std::vector<float> invalid_samples(6, 1.0f); // Invalid size

    SECTION("compute_magnitudes throws std::invalid_argument")
    {
        REQUIRE_THROWS_AS(mock_fft.compute_magnitudes(invalid_samples), std::invalid_argument);
    }

    SECTION("compute_complex throws std::invalid_argument")
    {
        REQUIRE_THROWS_AS(mock_fft.compute_complex(invalid_samples), std::invalid_argument);
    }
}