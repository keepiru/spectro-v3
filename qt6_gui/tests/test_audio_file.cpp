// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "adapters/audio_file_reader.h"
#include "audio_types.h"
#include "controllers/audio_file.h"
#include "models/audio_buffer.h"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <cstddef>
#include <iterator>
#include <qsignalspy.h>
#include <stdexcept>
#include <utility>
#include <vector>

/// @brief Mock implementation of IAudioFileReader for testing
/// Returns predefined audio data.
class MockAudioFileReader : public IAudioFileReader
{
  public:
    /// @brief Constructor
    /// @param channelCount Number of channels
    /// @param sampleRate Sample rate in Hz
    /// @param samples Interleaved audio samples to return
    MockAudioFileReader(ChannelCount channelCount,
                        SampleRate sampleRate,
                        std::vector<float> samples)
      : mSamples(std::move(samples))
      , mSampleRate(sampleRate)
      , mChannelCount(channelCount)
    {
        if (mSamples.size() % mChannelCount != 0) {
            throw std::invalid_argument("Sample count must be divisible by channel count");
        }
    }

    /// @brief Read interleaved audio samples
    /// @param aFrames Number of frames to read
    /// @return Vector of interleaved audio samples
    [[nodiscard]] std::vector<float> ReadInterleaved(FrameCount aFrames) override
    {
        const SampleCount kRequestedSamples = aFrames * mChannelCount;
        const size_t samplesToRead = std::min(kRequestedSamples.Get(), mSamples.size());
        const auto end = std::next(mSamples.begin(), static_cast<std::ptrdiff_t>(samplesToRead));
        std::vector<float> result(mSamples.begin(), end);
        mSamples.erase(mSamples.begin(), end);
        return result;
    }

    /// @brief Get the sample rate of the simulated audio file
    /// @return Sample rate in Hz
    [[nodiscard]] SampleRate GetSampleRate() const override { return mSampleRate; }

    /// @brief Get the number of channels in the simulated audio file
    /// @return Number of channels
    [[nodiscard]] ChannelCount GetChannelCount() const override { return mChannelCount; }

    /// @brief Get the total number of frames in the simulated audio file
    /// @return Total frames
    [[nodiscard]] FrameCount GetFrameCount() const override
    {
        return FrameCount{ mSamples.size() / mChannelCount };
    }

  private:
    std::vector<float> mSamples;
    SampleRate mSampleRate;
    ChannelCount mChannelCount;
};

TEST_CASE("AudioFile - construction", "[audio_file]")
{
    AudioBuffer buffer;
    const AudioFile audioFile(buffer);
}

TEST_CASE("AudioFile::LoadFileFromReader", "[audio_file]")
{
    AudioBuffer buffer;
    buffer.Reset(1, 44100);
    AudioFile audioFile(buffer);
    std::vector<int> progressCalls;
    const AudioFile::ProgressCallback progressCallback = [&](int aProgressPercent) {
        progressCalls.push_back(aProgressPercent);
    };

    SECTION("successful load")
    {
        MockAudioFileReader mockReader(2, 22050, { 0, 1, 2, 3, 4, 5 });

        audioFile.LoadFileFromReader(mockReader, progressCallback);

        CHECK(buffer.GetSampleRate() == 22050);
        CHECK(buffer.GetChannelCount() == 2);
        CHECK(buffer.GetFrameCount() == FrameCount(3));

        const auto kHave0 = buffer.GetSamples(0, SampleIndex(0), SampleCount(3));
        const auto kHave1 = buffer.GetSamples(1, SampleIndex(0), SampleCount(3));
        const auto kWant0 = std::vector<float>({ 0, 2, 4 });
        const auto kWant1 = std::vector<float>({ 1, 3, 5 });
        CHECK_THAT(kHave0, Catch::Matchers::RangeEquals(kWant0));
        CHECK_THAT(kHave1, Catch::Matchers::RangeEquals(kWant1));

        CHECK(progressCalls == std::vector<int>({ 100 }));
    }

    SECTION("empty file")
    {
        MockAudioFileReader mockReader(1, 8000, {});

        audioFile.LoadFileFromReader(mockReader, progressCallback);

        CHECK(buffer.GetSampleRate() == 8000);
        CHECK(buffer.GetChannelCount() == 1);
        CHECK(buffer.GetFrameCount() == FrameCount(0));
        CHECK(progressCalls == std::vector<int>({ 100 }));
    }

    SECTION("incremental progress updates")
    {
        // 12,345,678 samples total, 2 channels = 6,172,839 frames
        MockAudioFileReader mockReader(2, 44100, std::vector<float>(12345678));

        audioFile.LoadFileFromReader(mockReader, progressCallback);

        CHECK(progressCalls == std::vector<int>({ 16, 33, 50, 67, 84, 100 }));
    }
}

TEST_CASE("AudioFile::LoadFile", "[audio_file]")
{
    AudioBuffer buffer;
    AudioFile audioFile(buffer);
    std::vector<int> progressCalls;
    const AudioFile::ProgressCallback progressCallback = [&](int aProgressPercent) {
        progressCalls.push_back(aProgressPercent);
    };

    SECTION("loads a file")
    {
        auto result = audioFile.LoadFile("testdata/chirp.wav", progressCallback);
        REQUIRE(result.has_value());
        CHECK(buffer.GetSampleRate() == 44100);
        CHECK(buffer.GetChannelCount() == 1);
        CHECK(buffer.GetFrameCount() == FrameCount(4410));
        CHECK(progressCalls == std::vector<int>({ 100 }));
        const auto kHave = buffer.GetSamples(0, SampleIndex(0), SampleCount(10));
        const std::vector<float> kWant = { 0.0f,         0.014247104f, 0.028508738f, 0.042782005f,
                                           0.057063986f, 0.071351744f, 0.08564233f,  0.099932767f,
                                           0.11422006f,  0.128501192f };
        CHECK_THAT(kHave, Catch::Matchers::RangeEquals(kWant));
    }

    SECTION("AudioBuffer emits BufferReset")
    {
        // This is important to ensure UpdateColorMapDropdowns is called when loading a file.
        QSignalSpy bufferResetSpy(&buffer, &AudioBuffer::BufferReset);
        auto result = audioFile.LoadFile("testdata/chirp.wav", progressCallback);
        REQUIRE(result.has_value());
        CHECK(bufferResetSpy.count() == 1);
        CHECK(bufferResetSpy.takeFirst().takeFirst().toInt() == 1); // New channel count
    }

    SECTION("returns error on invalid file path")
    {
        auto result = audioFile.LoadFile("non_existent_file.wav", progressCallback);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == "Failed to open audio file non_existent_file.wav for reading: "
                                "System error : No such file or directory.");
    }

    SECTION("returns error on corrupt file")
    {
        auto result = audioFile.LoadFile("testdata/corrupt.wav", progressCallback);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() ==
              "Failed to open audio file testdata/corrupt.wav for reading: Format not recognised.");
    }
}