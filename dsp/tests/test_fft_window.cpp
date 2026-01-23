// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

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

TEST_CASE("FFTWindow::ComputeWindowCoefficients", "[fft_window]")
{

    /// @brief Helper to compute a window
    /// @param aSize Window size
    /// @param aType Window type
    /// @note ::ComputeWindowCoefficients is private, so we test its effects via Apply()
    auto computeCoefficients = [](const FFTSize aSize, FFTWindow::Type aType) {
        std::vector<float> input(aSize, 1.0f); // All ones

        // Apply window
        const FFTWindow window(aSize, aType);
        return window.Apply(input);
    };

    /// @brief Helper to check window coefficients against expected values
    /// @param aHave Computed coefficients
    /// @param aWant Expected coefficients
    auto checkWindowCoefficients = [](const std::vector<float>& aHave,
                                      const std::vector<float>& aWant) {
        REQUIRE(aHave.size() == aWant.size());
        for (size_t i = 0; i < aWant.size(); ++i) {
            CHECK_THAT(aHave[i], Catch::Matchers::WithinAbs(aWant[i], 1e-6f));
        }
    };

    /// @brief Helper to check symmetry of window coefficients
    /// @param aCoefficients Coefficients to check
    auto checkSymmetry = [](const std::vector<float>& aCoefficients) {
        const size_t kSize = aCoefficients.size();
        for (size_t i = 0; i < kSize / 2; ++i) {
            CHECK_THAT(aCoefficients[i],
                       Catch::Matchers::WithinAbs(aCoefficients[kSize - 1 - i], 1e-6f));
        }
    };

    SECTION("Rectangular window is identity")
    {
        constexpr FFTSize kSize = 1024;
        const auto kHave = computeCoefficients(kSize, FFTWindow::Type::Rectangular);
        const std::vector<float> kWant(kSize, 1.0f); // All ones
        checkWindowCoefficients(kHave, kWant);
    }

    SECTION("Hann")
    {
        SECTION("size 8")
        {
            const auto kHave = computeCoefficients(8, FFTWindow::Type::Hann);
            const std::vector<float> kWant = { 0.0f,       0.1882551f, 0.6112605f, 0.9504844f,
                                               0.9504844f, 0.6112605f, 0.1882551f, 0.0f };
            checkWindowCoefficients(kHave, kWant);
        }

        SECTION("size 1024")
        {
            constexpr FFTSize kSize = 1024;
            const auto kHave = computeCoefficients(kSize, FFTWindow::Type::Hann);

            checkSymmetry(kHave);

            // End coefficients are ~0; the last one is tested implicitly by symmetry
            CHECK_THAT(kHave[0], Catch::Matchers::WithinAbs(0.0f, 1e-6f));

            // Middle coefficient is 1.0
            CHECK_THAT(kHave[kSize / 2], Catch::Matchers::WithinAbs(1.0f, 1e-5f));
        }
    }
}

TEST_CASE("FFTWindow#Apply", "[fft_window]")
{
    SECTION("Input size mismatch throws")
    {
        FFTWindow const kWindow(4, FFTWindow::Type::Hann);
        std::vector<float> input = { 1.0f, 2.0f }; // Incorrect size

        REQUIRE_THROWS_AS(kWindow.Apply(input), std::invalid_argument);
    }
}

TEST_CASE("FFTWindow reduces spectral leakage", "[fft_window]")
{
    /// @brief Measure spectral leakage for a given window type
    /// @param aWindowType The type of window to test
    /// @return The total leakage power outside the main lobe
    /// @note Leakage is the sum of power outside the main lobe
    auto measureLeakage = [](const FFTWindow::Type aWindowType) {
        constexpr FFTSize kTransformSize = 1024;
        constexpr float kFrequency = 12.5f; // Frequency in bins, not an integer divisor of bins
        constexpr float kMainLobeDeviation = 3.0f; // Bins around the signal frequency to exclude
        FFTProcessor const kProcessor(kTransformSize);

        // Generate a sine wave that does not fit an integer number of cycles in the window
        std::vector<float> samples(kTransformSize);
        for (size_t i = 0; i < kTransformSize; ++i) {
            constexpr float kPI = std::numbers::pi_v<float>;
            samples[i] = std::sinf(2.0f * kPI * kFrequency * static_cast<float>(i) /
                                   static_cast<float>(kTransformSize));
        }

        // Apply window and compute spectrum
        const FFTWindow kWindow(kTransformSize, aWindowType);
        auto windowedSamples = kWindow.Apply(samples);
        const auto kSpectrum = kProcessor.ComputeMagnitudes(windowedSamples);

        // Measure leakage: sum of power outside the main lobe
        float leakageSum = 0.0f;
        for (size_t i = 0; i < kSpectrum.size(); ++i) {
            // Exclude main lobe around the signal frequency
            if (std::abs(static_cast<float>(i) - kFrequency) > kMainLobeDeviation) {
                leakageSum += kSpectrum[i] * kSpectrum[i]; // Power, not magnitude
            }
        }
        return leakageSum;
    };

    SECTION("Rectangular window leaks")
    {
        const float kLeakage = measureLeakage(FFTWindow::Type::Rectangular);
        REQUIRE_THAT(kLeakage, Catch::Matchers::WithinRel(17118.8f, 0.001f));
    }

    SECTION("Hann window reduces leakage")
    {
        const float kLeakage = measureLeakage(FFTWindow::Type::Hann);
        REQUIRE_THAT(kLeakage, Catch::Matchers::WithinRel(11.3f, 0.001f));
    }
}