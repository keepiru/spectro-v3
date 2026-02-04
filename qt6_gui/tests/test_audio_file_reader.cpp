// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "audio_types.h"
#include "controllers/audio_file_reader.h"
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <vector>

TEST_CASE("AudioFileReader", "[audio_file_reader]")
{
    auto readerResult = AudioFileReader::Open("testdata/chirp.wav");
    REQUIRE(readerResult.has_value());
    AudioFileReader& reader = *readerResult;

    SECTION("returns error on invalid file path")
    {
        auto result = AudioFileReader::Open("non_existent_file.wav");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == "Failed to open audio file non_existent_file.wav for reading: "
                                "System error : No such file or directory.");
    }

    SECTION("returns error on corrupt file")
    {
        auto result = AudioFileReader::Open("testdata/corrupt.wav");
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() ==
              "Failed to open audio file testdata/corrupt.wav for reading: Format not recognised.");
    }

    SECTION("GetSampleRate returns correct value")
    {
        REQUIRE(reader.GetSampleRate() == 44100);
    }

    SECTION("GetChannelCount returns correct value")
    {
        REQUIRE(reader.GetChannelCount() == 1);
    }

    SECTION("GetFrameCount returns correct value")
    {
        REQUIRE(reader.GetFrameCount() == FrameCount(4410));
    }

    SECTION("reads correct samples")
    {
        const size_t kFramesToRead = 10;
        const auto kHave = reader.ReadInterleaved(FrameCount(kFramesToRead));
        const std::vector<float> kWant = { 0.0f,         0.014247104f, 0.028508738f, 0.042782005f,
                                           0.057063986f, 0.071351744f, 0.08564233f,  0.099932767f,
                                           0.11422006f,  0.128501192f };
        REQUIRE(kHave == kWant);
    }

    SECTION("reads the whole file")
    {
        const auto kHave = reader.ReadInterleaved(FrameCount(10000)); // more than total frames
        REQUIRE(kHave.size() == 4410);
    }

    SECTION("reads the whole file in chunks")
    {
        REQUIRE(reader.ReadInterleaved(FrameCount(1000)).size() == 1000);
        REQUIRE(reader.ReadInterleaved(FrameCount(1000)).size() == 1000);
        REQUIRE(reader.ReadInterleaved(FrameCount(1000)).size() == 1000);
        REQUIRE(reader.ReadInterleaved(FrameCount(1000)).size() == 1000);
        REQUIRE(reader.ReadInterleaved(FrameCount(1000)).size() == 410);
    }
}
