#pragma once

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
