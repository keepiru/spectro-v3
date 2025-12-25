#include "ifft_processor.h"
#include "mock_fft_processor.h"
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

TEST_CASE("MockFFTProcessor returns fixed values", "[MockFFTProcessor]")
{
    const uint32_t kTransformSize = 8;
    MockFFTProcessor const kMockFFT(kTransformSize);

    std::vector<float> samples = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f };

    SECTION("ComputeMagnitudes returns fixed values")
    {
        REQUIRE(kMockFFT.ComputeMagnitudes(samples) ==
                std::vector<float>({ 0.0f, 1.0f, 2.0f, 3.0f, 4.0f }));
    }

    SECTION("ComputeDecibels returns fixed values")
    {
        REQUIRE(kMockFFT.ComputeDecibels(samples) ==
                std::vector<float>({ 0.0f, 1.0f, 2.0f, 3.0f, 4.0f }));
    }

    SECTION("ComputeComplex returns fixed complex values")
    {
        auto const complexOutput = kMockFFT.ComputeComplex(samples);
        CAPTURE(complexOutput);
        REQUIRE(complexOutput.size() == (samples.size() / 2) + 1);
        for (size_t i = 0; i < complexOutput.size(); ++i) {
            REQUIRE(complexOutput[i][0] == samples[i]); // Real part
            REQUIRE(complexOutput[i][1] == samples[i]); // Imaginary part
        }
    }
}

TEST_CASE("MockFFTProcessor throws on size mismatch", "[MockFFTProcessor]")
{
    MockFFTProcessor const mockFft(8);
    std::vector<float> invalidSamples(6, 1.0f); // Invalid size

    SECTION("ComputeMagnitudes throws std::invalid_argument")
    {
        REQUIRE_THROWS_AS(mockFft.ComputeMagnitudes(invalidSamples), std::invalid_argument);
    }

    SECTION("ComputeComplex throws std::invalid_argument")
    {
        REQUIRE_THROWS_AS(mockFft.ComputeComplex(invalidSamples), std::invalid_argument);
    }

    SECTION("ComputeDecibels throws std::invalid_argument")
    {
        REQUIRE_THROWS_AS(mockFft.ComputeDecibels(invalidSamples), std::invalid_argument);
    }
}

TEST_CASE("MockFFTProcessor::GetFactory", "[MockFFTProcessor]")
{
    const IFFTProcessorFactory factory = MockFFTProcessor::GetFactory();

    SECTION("Factory creates MockFFTProcessor instances")
    {
        const uint32_t kTransformSize = 32;
        auto processor = factory(kTransformSize);
        REQUIRE(processor != nullptr);
        REQUIRE(processor->GetTransformSize() == kTransformSize);
    }
}