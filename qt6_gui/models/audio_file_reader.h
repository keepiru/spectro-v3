#pragma once

#include <memory>
#include <sndfile.h>
#include <vector>

/// @brief Thin abstraction to read audio files.
class IAudioFileReader
{
  public:
    virtual ~IAudioFileReader() = default;

    /// @brief Read interleaved audio samples
    /// @param aFrames Number of frames to read
    /// @return Vector of interleaved audio samples
    [[nodiscard]] virtual std::vector<float> ReadInterleaved(size_t aFrames) = 0;

    /// @brief Get the sample rate of the audio file
    /// @return Sample rate in Hz
    [[nodiscard]] virtual int GetSampleRate() const = 0;

    /// @brief Get the number of channels in the audio file
    /// @return Number of channels
    [[nodiscard]] virtual int GetChannelCount() const = 0;

    /// @brief Get the total number of frames in the audio file
    /// @return Total frames
    [[nodiscard]] virtual size_t GetTotalFrames() const = 0;
};

/// @brief Audio file reader implementation using libsndfile
class AudioFileReader : public IAudioFileReader
{
  public:
    /// @brief Construct an AudioFileReader
    /// @param aFilePath Path to the audio file
    /// @throws std::runtime_error if the file cannot be opened
    explicit AudioFileReader(const std::string& aFilePath);

    ~AudioFileReader() override = default;
    [[nodiscard]] std::vector<float> ReadInterleaved(size_t aFrames) override;
    [[nodiscard]] int GetSampleRate() const override { return mSfInfo.samplerate; }
    [[nodiscard]] int GetChannelCount() const override { return mSfInfo.channels; }
    [[nodiscard]] size_t GetTotalFrames() const override
    {
        return static_cast<size_t>(mSfInfo.frames);
    }

  private:
    SF_INFO mSfInfo;
    using SfCloser = decltype(&sf_close);
    std::unique_ptr<SNDFILE, SfCloser> mSndFile{ nullptr, &sf_close };
};