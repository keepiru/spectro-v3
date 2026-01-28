// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include <audio_types.h>
#include <cstddef>
#include <memory>
#include <sndfile.h>
#include <string>
#include <vector>

/// @brief Thin abstraction to read audio files.
class IAudioFileReader
{
  public:
    virtual ~IAudioFileReader() = default;

    /// @brief Read interleaved audio samples
    /// @param aFrames Number of frames to read
    /// @return Vector of interleaved audio samples
    [[nodiscard]] virtual std::vector<float> ReadInterleaved(FrameCount aFrames) = 0;

    /// @brief Get the sample rate of the audio file
    /// @return Sample rate in Hz
    [[nodiscard]] virtual SampleRate GetSampleRate() const = 0;

    /// @brief Get the number of channels in the audio file
    /// @return Number of channels
    [[nodiscard]] virtual ChannelCount GetChannelCount() const = 0;

    /// @brief Get the number of frames in the audio file
    /// @return Total frames
    [[nodiscard]] virtual FrameCount GetFrameCount() const = 0;
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
    [[nodiscard]] std::vector<float> ReadInterleaved(FrameCount aFrames) override;
    [[nodiscard]] SampleRate GetSampleRate() const override { return mSfInfo.samplerate; }
    [[nodiscard]] ChannelCount GetChannelCount() const override { return mSfInfo.channels; }
    [[nodiscard]] FrameCount GetFrameCount() const override
    {
        return FrameCount{ static_cast<size_t>(mSfInfo.frames) };
    }

  private:
    SF_INFO mSfInfo;
    using SfCloser = decltype(&sf_close);
    std::unique_ptr<SNDFILE, SfCloser> mSndFile{ nullptr, &sf_close };
};