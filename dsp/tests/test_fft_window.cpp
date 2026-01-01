#include <audio_types.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <cstddef>
#include <fft_processor.h>
#include <fft_window.h>
#include <numbers>
#include <stdexcept>
#include <vector>

TEST_CASE("FFTWindow Constructor", "[fft_window]")
{
    SECTION("Valid sizes and types")
    {
        REQUIRE_NOTHROW(FFTWindow(256, FFTWindow::Type::Rectangular));
        REQUIRE_NOTHROW(FFTWindow(1024, FFTWindow::Type::Hann));
    }

    SECTION("Invalid sizes")
    {
        REQUIRE_THROWS_AS(FFTWindow(0, FFTWindow::Type::Hann), std::invalid_argument);
        REQUIRE_THROWS_AS(FFTWindow(-512, FFTWindow::Type::Hann), std::invalid_argument);
    }
}

TEST_CASE("FFTWindow#GetSize and GetType", "[fft_window]")
{
    FFTWindow const kWindow(1024, FFTWindow::Type::Hann);
    REQUIRE(kWindow.GetSize() == 1024);
    REQUIRE(kWindow.GetType() == FFTWindow::Type::Hann);
}

TEST_CASE("FFTWindow#Apply", "[fft_window]")
{
    SECTION("Input size mismatch throws")
    {
        FFTWindow const kWindow(4, FFTWindow::Type::Hann);
        std::vector<float> input = { 1.0f, 2.0f }; // Incorrect size

        REQUIRE_THROWS_AS(kWindow.Apply(input), std::invalid_argument);
    }

    SECTION("kRectangular window is identity")
    {
        FFTWindow const kWindow(4, FFTWindow::Type::Rectangular);

        std::vector<float> input = { 1.0f, 2.0f, 3.0f, 4.0f };
        auto output = kWindow.Apply(input);

        REQUIRE(output == std::vector<float>({ 1.0f, 2.0f, 3.0f, 4.0f }));
    }

    SECTION("kHann window coefficients")
    {
        FFTWindow const kWindow(8, FFTWindow::Type::Hann);
        std::vector<float> input = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
        auto output = kWindow.Apply(input);
        std::vector<float> expected = { 0.0f,       0.1882551f, 0.6112605f, 0.9504844f,
                                        0.9504844f, 0.6112605f, 0.1882551f, 0.0f };

        REQUIRE(output.size() == expected.size());
        for (size_t i = 0; i < output.size(); ++i) {
            REQUIRE_THAT(output[i], Catch::Matchers::WithinAbs(expected[i], 1e-6f));
        }
    }

    SECTION("Big kHann Window")
    {
        const FFTSize kSize = 1024;
        FFTWindow const window(kSize, FFTWindow::Type::Hann);
        std::vector<float> input(kSize, 1.0f); // All ones
        auto output = window.Apply(input);

        SECTION("First and last samples are zero")
        {
            REQUIRE_THAT(output[0], Catch::Matchers::WithinAbs(0.0f, 1e-6f));
            REQUIRE_THAT(output[kSize - 1], Catch::Matchers::WithinAbs(0.0f, 1e-6f));
        }

        SECTION("Middle sample is one")
        {
            REQUIRE_THAT(output[kSize / 2], Catch::Matchers::WithinAbs(1.0f, 1e-5f));
        }

        SECTION("Values are symmetric")
        {
            for (size_t i = 0; i < kSize / 2; ++i) {
                REQUIRE_THAT(output[i], Catch::Matchers::WithinAbs(output[kSize - 1 - i], 1e-6f));
            }
        }
    }
}

TEST_CASE("FFTWindow reduces spectral leakage", "[fft_window]")
{
    const FFTSize kTransformSize = 1024;
    const float kFrequency = 12.5; // Frequency in bins, not an integer divisor of bins
    FFTProcessor const kProcessor(kTransformSize);

    // Generate a sine wave that does not fit an integer number of cycles in the window
    std::vector<float> samples(kTransformSize);
    for (size_t i = 0; i < kTransformSize; ++i) {
        const auto kPI = std::numbers::pi_v<float>;
        samples[i] = std::sinf(2.0f * kPI * kFrequency * static_cast<float>(i) / kTransformSize);
    }

    // Compute spectrum without windowing
    auto spectrumNoWindow = kProcessor.ComputeMagnitudes(samples);

    // Apply kHann window and compute spectrum
    FFTWindow const kHannWindow(kTransformSize, FFTWindow::Type::Hann);
    auto windowedSamples = kHannWindow.Apply(samples);
    auto const kSpectrumWithWindow = kProcessor.ComputeMagnitudes(windowedSamples);

    // Check that the peak magnitude is lower with windowing (reduced leakage)
    auto measureLeakage = [kFrequency](const std::vector<float>& aSpectrum) {
        float leakageSum = 0.0f;
        for (size_t i = 0; i < aSpectrum.size(); ++i) {
            // Exclude main lobe around the signal frequency
            if (std::abs(static_cast<float>(i) - kFrequency) > 3.0f) {
                leakageSum += aSpectrum[i] * aSpectrum[i]; // Power, not magnitude
            }
        }
        return leakageSum;
    };

    float const kLeakageNoWindow = measureLeakage(spectrumNoWindow);
    float const kLeakageWithWindow = measureLeakage(kSpectrumWithWindow);

    // Expect significant leakage reduction
    REQUIRE(kLeakageWithWindow < kLeakageNoWindow * 0.01f);
}