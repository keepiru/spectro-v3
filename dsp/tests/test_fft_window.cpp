#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <fft_processor.h>
#include <fft_window.h>
#include <iostream>
#include <math.h>
#include <vector>

TEST_CASE("FFTWindow Constructor", "[fft_window]")
{
    SECTION("Valid sizes and types")
    {
        REQUIRE_NOTHROW(FFTWindow(256, FFTWindow::Type::Rectangular));
        REQUIRE_NOTHROW(FFTWindow(1024, FFTWindow::Type::Hann));
    }

    SECTION("Invalid size (zero)")
    {
        REQUIRE_THROWS_AS(FFTWindow(0, FFTWindow::Type::Hann), std::invalid_argument);
    }
}

TEST_CASE("FFTWindow#getSize and getType", "[fft_window]")
{
    FFTWindow window(1024, FFTWindow::Type::Hann);
    REQUIRE(window.getSize() == 1024);
    REQUIRE(window.getType() == FFTWindow::Type::Hann);
}

TEST_CASE("FFTWindow#apply", "[fft_window]")
{
    SECTION("Input size mismatch throws")
    {
        FFTWindow window(4, FFTWindow::Type::Hann);
        std::vector<float> input = { 1.0f, 2.0f }; // Incorrect size

        REQUIRE_THROWS_AS(window.apply(input), std::invalid_argument);
    }

    SECTION("Rectangular window is identity")
    {
        FFTWindow window(4, FFTWindow::Type::Rectangular);

        std::vector<float> input = { 1.0f, 2.0f, 3.0f, 4.0f };
        auto output = window.apply(input);

        REQUIRE(output == std::vector<float>({ 1.0f, 2.0f, 3.0f, 4.0f }));
    }

    SECTION("Hann window coefficients")
    {
        FFTWindow window(8, FFTWindow::Type::Hann);
        std::vector<float> input = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
        auto output = window.apply(input);
        std::vector<float> expected = { 0.0f,       0.1882551f, 0.6112605f, 0.9504844f,
                                        0.9504844f, 0.6112605f, 0.1882551f, 0.0f };

        REQUIRE(output.size() == expected.size());
        for (size_t i = 0; i < output.size(); ++i) {
            REQUIRE_THAT(output[i], Catch::Matchers::WithinAbs(expected[i], 1e-6f));
        }
    }

    SECTION("Big Hann Window")
    {
        const uint32_t size = 1024;
        FFTWindow window(size, FFTWindow::Type::Hann);
        std::vector<float> input(size, 1.0f); // All ones
        auto output = window.apply(input);

        SECTION("First and last samples are zero")
        {
            REQUIRE_THAT(output[0], Catch::Matchers::WithinAbs(0.0f, 1e-6f));
            REQUIRE_THAT(output[size - 1], Catch::Matchers::WithinAbs(0.0f, 1e-6f));
        }

        SECTION("Middle sample is one")
        {
            REQUIRE_THAT(output[size / 2], Catch::Matchers::WithinAbs(1.0f, 1e-5f));
        }

        SECTION("Values are symmetric")
        {
            for (size_t i = 0; i < size / 2; ++i) {
                REQUIRE_THAT(output[i], Catch::Matchers::WithinAbs(output[size - 1 - i], 1e-6f));
            }
        }
    }
}

TEST_CASE("FFTWindow reduces spectral leakage", "[fft_window]")
{
    const uint32_t transform_size = 1024;
    const float frequency = 12.5; // Frequency in bins, not an integer divisor of bins
    FFTProcessor processor(transform_size);

    // Generate a sine wave that does not fit an integer number of cycles in the window
    std::vector<float> samples(transform_size);
    for (size_t i = 0; i < transform_size; ++i) {
        samples[i] = std::sin(2.0f * std::numbers::pi * frequency * i / transform_size);
    }

    // Compute spectrum without windowing
    auto spectrum_no_window = processor.compute_magnitudes(samples);

    // Apply Hann window and compute spectrum
    FFTWindow hann_window(transform_size, FFTWindow::Type::Hann);
    auto windowed_samples = hann_window.apply(samples);
    auto spectrum_with_window = processor.compute_magnitudes(windowed_samples);

    // Check that the peak magnitude is lower with windowing (reduced leakage)
    auto measure_leakage = [frequency](const std::vector<float>& spectrum) {
        float leakage_sum = 0.0f;
        for (size_t i = 0; i < spectrum.size(); ++i) {
            // Exclude main lobe around the signal frequency
            if (abs(static_cast<float>(i) - frequency) > 3.0f) {
                leakage_sum += spectrum[i] * spectrum[i]; // Power, not magnitude
            }
        }
        return leakage_sum;
    };

    float leakage_no_window = measure_leakage(spectrum_no_window);
    float leakage_with_window = measure_leakage(spectrum_with_window);

    // Expect significant leakage reduction
    REQUIRE(leakage_with_window < leakage_no_window * 0.01f);
}