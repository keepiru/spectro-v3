#include "audio_types.h"
#include <catch2/catch_test_macros.hpp>
#include <sample_buffer.h>
#include <stdexcept>
#include <vector>

TEST_CASE("SampleBuffer basic functionality", "[SampleBuffer]")
{
    const SampleRate kSampleRate = 44100;
    SampleBuffer buffer(kSampleRate);

    std::vector<float> kSamples = { 0.1f, 0.2f, 0.3f, 0.4f };
    buffer.AddSamples(kSamples);

    SECTION("Check properties")
    {
        REQUIRE(buffer.GetSampleRate() == kSampleRate);
        REQUIRE(buffer.GetSampleCount() == SampleCount(kSamples.size()));
    }

    SECTION("Retrieve all samples")
    {
        auto retrieved = buffer.GetSamples(SampleIndex(0), SampleCount(kSamples.size()));
        REQUIRE(retrieved == kSamples);
    }

    SECTION("Retrieve partial samples")
    {
        auto retrieved = buffer.GetSamples(SampleIndex(1), SampleCount(2));
        REQUIRE(retrieved == std::vector<float>({ 0.2f, 0.3f }));
    }

    SECTION("Throws when retrieving beyond buffer size")
    {
        REQUIRE_THROWS_AS(buffer.GetSamples(SampleIndex(2), SampleCount(4)), std::out_of_range);
    }

    SECTION("Append more samples")
    {
        std::vector<float> const kNewSamples = { 0.5f, 0.6f };
        buffer.AddSamples(kNewSamples);

        auto const kRetrieved = buffer.GetSamples(SampleIndex(0), SampleCount(kSamples.size() + kNewSamples.size()));
        REQUIRE(kRetrieved == std::vector<float>({ 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f }));
    }
}