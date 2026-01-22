// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include <audio_types.h>
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <cstddef>
#include <fft_processor.h>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

TEST_CASE("Constructor succeeds", "[fft]")
{
    const FFTSize kTransformSize = 1024;
    FFTProcessor const kProcessor(kTransformSize);

    REQUIRE(kProcessor.GetTransformSize() == kTransformSize);

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
    const FFTSize kTransformSize = 512;
    FFTProcessor processor1(kTransformSize);

    FFTProcessor processor2(std::move(processor1));
    REQUIRE(processor2.GetTransformSize() == kTransformSize);

    FFTProcessor processor3(256);
    processor3 = std::move(processor2);
    REQUIRE(processor3.GetTransformSize() == kTransformSize);

    static_assert(!std::is_copy_constructible_v<FFTProcessor>);
    static_assert(!std::is_copy_assignable_v<FFTProcessor>);
    static_assert(std::is_move_constructible_v<FFTProcessor>);
    static_assert(std::is_move_assignable_v<FFTProcessor>);
}

TEST_CASE("FFTProcessor#ComputeComplex", "[fft]")
{
    const FFTSize kTransformSize = 8; // Small for testing
    FFTProcessor const kProcessor(kTransformSize);

    SECTION("Throws on input size mismatch", "[fft]")
    {
        std::vector<float> samples(kTransformSize - 1, 0.0f); // Incorrect size
        REQUIRE_THROWS_AS(kProcessor.ComputeComplex(samples), std::invalid_argument);
    }

    SECTION("Compute complex FFT output", "[fft]")
    {
        std::vector<float> samples = { 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f };
        const auto kFFTOutput = kProcessor.ComputeComplex(samples);

        REQUIRE(kFFTOutput.size() == (kTransformSize / 2) + 1);
        REQUIRE_THAT(kFFTOutput[0][0], Catch::Matchers::WithinRel(0.0));  // DC real part
        REQUIRE_THAT(kFFTOutput[0][1], Catch::Matchers::WithinRel(0.0));  // DC imag part
        REQUIRE_THAT(kFFTOutput[1][0], Catch::Matchers::WithinRel(0.0));  // Bin 1 real part
        REQUIRE_THAT(kFFTOutput[1][1], Catch::Matchers::WithinRel(0.0));  // Bin 1 imag part
        REQUIRE_THAT(kFFTOutput[2][0], Catch::Matchers::WithinRel(0.0));  // Bin 2 real part
        REQUIRE_THAT(kFFTOutput[2][1], Catch::Matchers::WithinRel(-4.0)); // Bin 2 imag part
        REQUIRE_THAT(kFFTOutput[3][0], Catch::Matchers::WithinRel(0.0));  // Bin 3 real part
        REQUIRE_THAT(kFFTOutput[3][1], Catch::Matchers::WithinRel(0.0));  // Bin 3 imag part
        REQUIRE_THAT(kFFTOutput[4][0], Catch::Matchers::WithinRel(0.0));  // Nyquist real part
        REQUIRE_THAT(kFFTOutput[4][1], Catch::Matchers::WithinRel(0.0));  // Nyquist imag part
    }
}

TEST_CASE("FFTProcessor#ComputeMagnitudes", "[fft]")
{
    const FFTSize kTransformSize = 8; // Small for testing
    FFTProcessor const kProcessor(kTransformSize);

    SECTION("Throws on input size mismatch", "[fft]")
    {
        std::vector<float> samples(kTransformSize - 1, 0.0f); // Incorrect size
        REQUIRE_THROWS_AS(kProcessor.ComputeMagnitudes(samples), std::invalid_argument);
    }

    SECTION("Computes DC component for constant signal", "[fft]")
    {
        std::vector<float> samples(kTransformSize, 1.0f);
        auto spectrum = kProcessor.ComputeMagnitudes(samples);

        REQUIRE(spectrum.size() == (kTransformSize / 2) + 1);
        REQUIRE_THAT(spectrum[0], Catch::Matchers::WithinRel(8.0));
    }

    SECTION("Computes peak frequency of sine wave", "[fft]")
    {
        const float kFrequency = 1.0f; // 1 cycle over 8 samples
        std::vector<float> samples(kTransformSize);
        for (size_t i = 0; i < kTransformSize; ++i) {
            const auto kPI = std::numbers::pi_v<float>;
            samples[i] = std::sinf(2.0f * kPI * kFrequency * static_cast<float>(i) /
                                   static_cast<float>(kTransformSize));
        }

        auto spectrum = kProcessor.ComputeMagnitudes(samples);

        REQUIRE(spectrum.size() == (kTransformSize / 2) + 1);
        REQUIRE_THAT(spectrum[0], Catch::Matchers::WithinAbs(0.0, 0.00001)); // No DC component
        REQUIRE_THAT(spectrum[1], Catch::Matchers::WithinAbs(4.0, 0.00001)); // Peak at bin 1
        REQUIRE_THAT(spectrum[2], Catch::Matchers::WithinAbs(0.0, 0.00001)); // No bin 2 component
        REQUIRE_THAT(spectrum[3], Catch::Matchers::WithinAbs(0.0, 0.00001)); // No bin 3 component
        REQUIRE_THAT(spectrum[4], Catch::Matchers::WithinAbs(0.0, 0.00001)); // No Nyquist component
    }
}

TEST_CASE("FFTProcessor#ComputeDecibels", "[fft]")
{
    const FFTSize kTransformSize = 8; // Small for testing
    FFTProcessor const kProcessor(kTransformSize);

    SECTION("Throws on input size mismatch", "[fft]")
    {
        std::vector<float> samples(kTransformSize - 1, 0.0f); // Incorrect size
        REQUIRE_THROWS_AS(kProcessor.ComputeDecibels(samples), std::invalid_argument);
    }

    SECTION("Computes DC component for constant signal", "[fft]")
    {
        std::vector<float> samples(kTransformSize, 1.0f);
        auto spectrum = kProcessor.ComputeDecibels(samples);

        CAPTURE(spectrum);
        REQUIRE(spectrum.size() == 5);
        // 20*log10(8) = 18.061f
        constexpr float negInf = -std::numeric_limits<float>::infinity();
        REQUIRE_THAT(spectrum[0], Catch::Matchers::WithinAbs(18.061f, 0.001f));
        REQUIRE_THAT(spectrum[1], Catch::Matchers::WithinRel(negInf));
        REQUIRE_THAT(spectrum[2], Catch::Matchers::WithinRel(negInf));
        REQUIRE_THAT(spectrum[3], Catch::Matchers::WithinRel(negInf));
        REQUIRE_THAT(spectrum[4], Catch::Matchers::WithinRel(negInf));
    }

    SECTION("Computes peak frequency of sine wave", "[fft]")
    {
        const float kFrequency = 1.0f; // 1 cycle over 8 samples
        std::vector<float> samples(kTransformSize);
        for (size_t i = 0; i < kTransformSize; ++i) {
            const auto kPI = std::numbers::pi_v<float>;
            samples[i] = std::sinf(2.0f * kPI * kFrequency * static_cast<float>(i) /
                                   static_cast<float>(kTransformSize));
        }

        auto spectrum = kProcessor.ComputeDecibels(samples);
        CAPTURE(spectrum);
        REQUIRE(spectrum.size() == (kTransformSize / 2) + 1);
        REQUIRE(spectrum[0] < -100.0f); // No DC component
        REQUIRE_THAT(spectrum[1], Catch::Matchers::WithinAbs(12.041f, 0.001f));
        REQUIRE(spectrum[2] < -100.0f); // No bin 2 component
        REQUIRE(spectrum[3] < -100.0f); // No bin 3 component
        REQUIRE(spectrum[4] < -100.0f); // No Nyquist component
    }
}