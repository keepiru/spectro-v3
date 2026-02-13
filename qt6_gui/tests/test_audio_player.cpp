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

namespace {

struct AudioPlayerTestFixture
{
    AudioBuffer audio_buffer;
    AudioPlayer::AudioSinkFactory audio_sink_factory = []() {
        return std::make_unique<StubAudioSink>();
    };
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