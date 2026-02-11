// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "audio_types.h"
#include "include/global_constants.h"
#include "models/audio_buffer.h"
#include <QAudioFormat>
#include <QSignalSpy>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <stdexcept>
#include <vector>

TEST_CASE("AudioBuffer constructor", "[audio_buffer]")
{
    AudioBuffer const buffer;
    REQUIRE(buffer.GetChannelCount() == 2);
    REQUIRE(buffer.GetSampleRate() == 44100);
}

TEST_CASE("AudioBuffer::Reset throws on invalid arguments", "[audio_buffer]")
{
    AudioBuffer buffer;

    REQUIRE_THROWS_AS(buffer.Reset(-1, 44100), std::invalid_argument);
    REQUIRE_THROWS_AS(buffer.Reset(0, 44100), std::invalid_argument);
    REQUIRE_THROWS_AS(buffer.Reset(GKMaxChannels + 1, 44100), std::invalid_argument);
    REQUIRE_THROWS_AS(buffer.Reset(2, 0), std::invalid_argument);
}

TEST_CASE("AudioBuffer::AddSamples succeeds with valid size", "[audio_buffer]")
{
    AudioBuffer buffer;
    buffer.AddSamples({ 1, 2, 3, 4 });
}

TEST_CASE("AudioBuffer::AddSamples throws on invalid size", "[audio_buffer]")
{
    AudioBuffer buffer;
    REQUIRE_THROWS_AS(buffer.AddSamples({ 1, 2, 3, 4, 5 }), std::invalid_argument);
}

TEST_CASE("AudioBuffer::GetSamples throws if insufficient samples", "[audio_buffer]")
{
    AudioBuffer buffer;
    buffer.AddSamples({ 1, 2, 3, 4 });
    REQUIRE_THROWS_AS((void)buffer.GetSamples(1, SampleIndex(1), SampleCount(4)),
                      std::out_of_range);
}

TEST_CASE("AudioBuffer::GetSamples throws on invalid channel index", "[audio_buffer]")
{
    AudioBuffer const buffer;
    (void)buffer.GetSamples(1, SampleIndex(0), SampleCount(0)); // No exception
    REQUIRE_THROWS_AS((void)buffer.GetSamples(2, SampleIndex(0), SampleCount(0)),
                      std::out_of_range);
}

TEST_CASE("AudioBuffer::AddSamples deinterleaves to channel buffers", "[audio_buffer]")
{
    AudioBuffer buffer;
    buffer.AddSamples({ 1, 2, 3, 4 });

    const auto kHave0 = buffer.GetSamples(0, SampleIndex(0), SampleCount(2));
    const auto kHave1 = buffer.GetSamples(1, SampleIndex(0), SampleCount(2));
    const auto kWant0 = std::vector<float>({ 1, 3 });
    const auto kWant1 = std::vector<float>({ 2, 4 });
    REQUIRE_THAT(kHave0, Catch::Matchers::RangeEquals(kWant0));
    REQUIRE_THAT(kHave1, Catch::Matchers::RangeEquals(kWant1));
}

TEST_CASE("AudioBuffer::AddSamples emits signal", "[audio_buffer]")
{
    AudioBuffer buffer;
    QSignalSpy spy(&buffer, &AudioBuffer::DataAvailable);
    buffer.AddSamples({ 1, 2, 3, 4 });

    REQUIRE(spy.count() == 1);
    auto firstCallArgs = spy.takeFirst();
    auto have = firstCallArgs.takeFirst().value<FrameCount>();
    REQUIRE(have == FrameCount(2));
}

TEST_CASE("AudioBuffer::Reset clears samples", "[audio_buffer]")
{
    AudioBuffer buffer;
    buffer.AddSamples({ 1, 2, 3, 4 });

    REQUIRE(buffer.GetFrameCount() == FrameCount(2));

    buffer.Reset(2, 44100);
    REQUIRE(buffer.GetFrameCount() == FrameCount(0));
    REQUIRE_THROWS_AS((void)buffer.GetSamples(0, SampleIndex(0), SampleCount(1)),
                      std::out_of_range);

    // Also test after changing channel count
    buffer.AddSamples({ 5, 6, 7, 8 });
    buffer.Reset(1, 44100);

    REQUIRE(buffer.GetFrameCount() == FrameCount(0));
}

TEST_CASE("AudioBuffer::Reset changes channel count and sample rate", "[audio_buffer]")
{
    AudioBuffer buffer;

    REQUIRE(buffer.GetChannelCount() == 2);
    REQUIRE(buffer.GetSampleRate() == 44100);

    buffer.Reset(1, 22050);

    REQUIRE(buffer.GetChannelCount() == 1);
    REQUIRE(buffer.GetSampleRate() == 22050);
}

TEST_CASE("AudioBuffer::Reset emits buffer reset signal", "[audio_buffer]")
{
    AudioBuffer buffer;
    const QSignalSpy spy(&buffer, &AudioBuffer::BufferReset);

    REQUIRE(spy.count() == 0);

    buffer.Reset(2, 44100);
    REQUIRE(spy.count() == 1);

    buffer.AddSamples({ 1, 2, 3, 4 });
    REQUIRE(spy.count() == 1);

    buffer.Reset(2, 44100);
    REQUIRE(spy.count() == 2);
}

TEST_CASE("AudioBuffer::BytesPerFrame returns correct value", "[audio_buffer]")
{
    AudioBuffer buffer;

    SECTION("Default format")
    {
        CHECK(buffer.GetBytesPerFrame() == 8);
    }

    SECTION("After reset to 1 channel, 22050 Hz")
    {
        buffer.Reset(1, 22050);
        CHECK(buffer.GetBytesPerFrame() == 4);
    }
}

TEST_CASE("AudioBuffer::GetAudioFormat returns correct format", "[audio_buffer]")
{
    AudioBuffer buffer;

    SECTION("Default format")
    {
        const auto format = buffer.GetAudioFormat();

        CHECK(format.channelCount() == 2);
        CHECK(format.sampleRate() == 44100);
        CHECK(format.sampleFormat() == QAudioFormat::Float);
        CHECK(format.bytesPerSample() == 4);
        CHECK(format.bytesPerFrame() == 8);
    }

    SECTION("After reset to 1 channel, 22050 Hz")
    {
        buffer.Reset(1, 22050);
        const auto format = buffer.GetAudioFormat();

        CHECK(format.channelCount() == 1);
        CHECK(format.sampleRate() == 22050);
        CHECK(format.sampleFormat() == QAudioFormat::Float);
        CHECK(format.bytesPerSample() == 4);
        CHECK(format.bytesPerFrame() == 4);
    }
}