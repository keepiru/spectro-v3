// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "audio_types.h"
#include "controllers/audio_player.h"
#include "models/audio_buffer.h"
#include "tests/stub_audio_sink.h"
#include <QIODevice>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <utility>
#include <vector>

namespace {

struct AudioPlayerTestFixture
{
    AudioBuffer audio_buffer;
    std::unique_ptr<StubAudioSink> audio_sink = std::make_unique<StubAudioSink>();
    StubAudioSink& audio_sink_ref = *audio_sink; // Save a ref to manipulate the sink in tests
    // Factory that returns our stub sink and allows us to keep a reference to
    // it for testing.  Note that the factory can only be called once since it
    // moves the unique_ptr, but that's fine for our tests since we only start
    // playback once per test.
    AudioPlayer::AudioSinkFactory audio_sink_factory = [this]() { return std::move(audio_sink); };
    AudioPlayer audio_player{ audio_buffer, audio_sink_factory };
};

} // namespace

TEST_CASE("AudioPlayer constructor", "[audio_player]")
{
    const AudioPlayerTestFixture fixture;
}

TEST_CASE("AudioPlayer::Start", "[audio_player]")
{
    AudioPlayerTestFixture fixture;

    SECTION("starts successfully with valid audio buffer")
    {
        const auto kResult = fixture.audio_player.Start(FrameIndex{ 0 });
        CHECK(kResult);
        CHECK(fixture.audio_player.IsPlaying());
    }

    SECTION("returns error if seek fails")
    {
        // Requesting start beyond the end of the buffer should cause seek to fail
        const auto kResult = fixture.audio_player.Start(FrameIndex{ 1 });
        CHECK_FALSE(kResult);
        CHECK(kResult.error() == "Failed to seek to start frame in AudioBufferQIODevice");
        CHECK_FALSE(fixture.audio_player.IsPlaying());
    }

    SECTION("returns error if audio sink creation fails")
    {
        AudioPlayerTestFixture brokenFactoryFixture{
            .audio_buffer = AudioBuffer{}, .audio_sink_factory = []() { return nullptr; }
        };

        const auto kResult = brokenFactoryFixture.audio_player.Start(FrameIndex{ 0 });

        CHECK_FALSE(kResult);
        CHECK(kResult.error() == "Failed to create audio sink");
        CHECK_FALSE(brokenFactoryFixture.audio_player.IsPlaying());
    }
}

TEST_CASE("AudioPlayer::Stop", "[audio_player]")
{
    AudioPlayerTestFixture fixture;

    SECTION("stops successfully when playing")
    {
        REQUIRE(fixture.audio_player.Start(FrameIndex{ 0 }));
        REQUIRE(fixture.audio_player.IsPlaying());

        fixture.audio_player.Stop();
        REQUIRE_FALSE(fixture.audio_player.IsPlaying());
    }

    SECTION("does nothing when not playing")
    {
        REQUIRE_FALSE(fixture.audio_player.IsPlaying());
        fixture.audio_player.Stop();
        REQUIRE_FALSE(fixture.audio_player.IsPlaying());
    }
}

TEST_CASE("AudioPlayer::CurrentFrame", "[audio_player]")
{
    AudioPlayerTestFixture fixture;

    SECTION("returns error when not playing")
    {
        // Check assumptions
        REQUIRE_FALSE(fixture.audio_player.IsPlaying());

        const auto kResult = fixture.audio_player.CurrentFrame();
        CHECK_FALSE(kResult);
    }

    SECTION("returns start frame when no time has elapsed")
    {
        REQUIRE(fixture.audio_player.Start(FrameIndex{ 0 }));

        const auto kResult = fixture.audio_player.CurrentFrame();
        REQUIRE(kResult);
        CHECK(kResult == FrameIndex{ 0 });
    }

    SECTION("calculates current frame correctly with elapsed time")
    {
        const FrameIndex kStartFrame{ 0 };
        REQUIRE(fixture.audio_player.Start(kStartFrame));

        // Simulate 1 second of playback (1,000,000 microseconds)
        // With default sample rate of 44'100 Hz, this should advance 44'100 frames
        fixture.audio_sink_ref.SetProcessedUSecs(1'000'000);
        const auto kResult1 = fixture.audio_player.CurrentFrame();
        CHECK(kResult1 == FrameIndex{ 44'100 });

        // Simulate 0.5 seconds of playback (500,000 microseconds)
        // This should advance 22'050 frames from the start
        fixture.audio_sink_ref.SetProcessedUSecs(500'000);
        const auto kResult2 = fixture.audio_player.CurrentFrame();
        CHECK(kResult2 == FrameIndex{ 22'050 });

        // Simulate 100ms of playback (100,000 microseconds)
        // This should advance 4'410 frames from the start
        fixture.audio_sink_ref.SetProcessedUSecs(100'000);
        const auto kResult3 = fixture.audio_player.CurrentFrame();
        CHECK(kResult3 == FrameIndex{ 4'410 });
    }

    SECTION("calculates current frame correctly with non-zero start frame")
    {
        fixture.audio_buffer.Reset(2, 44'100);

        // Add some dummy audio data so we can start from a non-zero frame
        const std::vector<float> samples(88'200, 0.0f); // 1 second of silence (44.1k frames)
        fixture.audio_buffer.AddSamples(samples);

        // Start playing from frame 10'000
        const FrameIndex kStartFrame{ 10'000 };
        REQUIRE(fixture.audio_player.Start(kStartFrame));

        // Simulate 0.5 seconds of playback (500,000 microseconds)
        fixture.audio_sink_ref.SetProcessedUSecs(500'000);

        // This should get us to frame 10'000 + 22'050
        const auto kResult = fixture.audio_player.CurrentFrame();
        CHECK(kResult == FrameIndex{ 10'000 + 22'050 });
    }
}