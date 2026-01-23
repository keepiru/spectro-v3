// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "audio_types.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
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
        REQUIRE_THAT(retrieved, Catch::Matchers::RangeEquals(kSamples));
    }

    SECTION("Retrieve partial samples")
    {
        auto retrieved = buffer.GetSamples(SampleIndex(1), SampleCount(2));
        const std::vector<float> kExpected = { 0.2f, 0.3f };
        REQUIRE_THAT(retrieved, Catch::Matchers::RangeEquals(kExpected));
    }

    SECTION("Retrieve zero samples")
    {
        auto retrieved = buffer.GetSamples(SampleIndex(2), SampleCount(0));
        REQUIRE(retrieved.empty());
    }

    SECTION("Retrieve zero samples from an empty buffer")
    {
        const SampleBuffer emptyBuffer(kSampleRate);
        auto retrieved = emptyBuffer.GetSamples(SampleIndex(0), SampleCount(0));
        REQUIRE(retrieved.empty());
    }

    SECTION("Throws when retrieving beyond buffer size")
    {
        REQUIRE_THROWS_AS(buffer.GetSamples(SampleIndex(2), SampleCount(4)), std::out_of_range);
    }

    SECTION("Append more samples")
    {
        std::vector<float> const kNewSamples = { 0.5f, 0.6f };
        buffer.AddSamples(kNewSamples);

        auto const kRetrieved =
          buffer.GetSamples(SampleIndex(0), SampleCount(kSamples.size() + kNewSamples.size()));
        const std::vector<float> kWant = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f };
        REQUIRE_THAT(kRetrieved, Catch::Matchers::RangeEquals(kWant));
    }
}