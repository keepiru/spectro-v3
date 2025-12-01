#include "mock_fft_processor.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("MockFFTProcessor returns fixed values", "[MockFFTProcessor]")
{
    const uint32_t num_bins = 8;
    MockFFTProcessor mock_fft(num_bins);

    std::vector<float> results1 = { 0.1, 0.2, 0.3, 0.4, 0.5 };
    std::vector<float> results2 = { 0.6, 0.7, 0.8, 0.9, 1.0 };
    std::vector<float> results3 = { 1.1, 1.2, 1.3, 1.4, 1.5 };
    mock_fft.addResult(results1);
    mock_fft.addResult(results2);
    mock_fft.addResult(results3);

    std::vector<float> input_samples(num_bins, 1.0f); // Input samples (all ones)

    SECTION("compute_magnitudes returns fixed values")
    {
        REQUIRE(mock_fft.compute_magnitudes(input_samples) == results1);
        REQUIRE(mock_fft.compute_magnitudes(input_samples) == results2);
        REQUIRE(mock_fft.compute_magnitudes(input_samples) == results3);
    }

    SECTION("compute_complex returns fixed complex values")
    {
        auto all_results = { results1, results2, results3 };
        for (const auto& expected : all_results) {
            auto complex_output = mock_fft.compute_complex(input_samples);
            CAPTURE(complex_output);
            REQUIRE(complex_output.size() == expected.size());
            for (size_t i = 0; i < expected.size(); ++i) {
                REQUIRE(complex_output[i][0] == expected[i]); // Real part
                REQUIRE(complex_output[i][1] == expected[i]); // Imaginary part
            }
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