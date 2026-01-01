#include "controllers/audio_file.h"
#include "include/global_constants.h"
#include "models/audio_file_reader.h"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>

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
      : mChannelCount(channelCount)
      , mSampleRate(sampleRate)
      , mSamples(std::move(samples))
    {
        if (mSamples.size() % mChannelCount != 0) {
            throw std::invalid_argument("Sample count must be divisible by channel count");
        }
    }

    /// @brief Read interleaved audio samples
    /// @param aFrames Number of frames to read
    /// @return Vector of interleaved audio samples
    [[nodiscard]] std::vector<float> ReadInterleaved(size_t aFrames) override
    {
        const size_t samplesToRead = std::min(aFrames * mChannelCount, mSamples.size());
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
    [[nodiscard]] size_t GetFrameCount() const override { return mSamples.size() / mChannelCount; }

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

TEST_CASE("AudioFile - load file", "[audio_file]")
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

        REQUIRE(audioFile.LoadFile(mockReader, progressCallback));

        CHECK(buffer.GetSampleRate() == 22050);
        CHECK(buffer.GetChannelCount() == 2);
        CHECK(buffer.GetFrameCount() == 3);
        CHECK(buffer.GetSamples(0, 0, 3) == std::vector<float>({ 0, 2, 4 }));
        CHECK(buffer.GetSamples(1, 0, 3) == std::vector<float>({ 1, 3, 5 }));
        CHECK(progressCalls == std::vector<int>({ 100 }));
    }

    SECTION("empty file")
    {
        MockAudioFileReader mockReader(1, 8000, {});

        REQUIRE(audioFile.LoadFile(mockReader, progressCallback));

        CHECK(buffer.GetSampleRate() == 8000);
        CHECK(buffer.GetChannelCount() == 1);
        CHECK(buffer.GetFrameCount() == 0);
        CHECK(progressCalls == std::vector<int>({ 100 }));
    }

    SECTION("incremental progress updates")
    {
        // 12,345,678 samples total, 2 channels = 6,172,839 frames
        MockAudioFileReader mockReader(2, 44100, std::vector<float>(12345678));

        REQUIRE(audioFile.LoadFile(mockReader, progressCallback));

        CHECK(progressCalls == std::vector<int>({ 16, 33, 50, 67, 84, 100 }));
    }
}