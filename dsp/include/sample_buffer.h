// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once
#include "audio_types.h"
#include <span>
#include <vector>

/// @brief Audio sample storage.
///
/// Stores single-channel audio.  Supports random access for scrubbing.
class SampleBuffer
{
  public:
    /// @brief Construct a SampleBuffer.
    /// @param aSampleRate Sample rate in Hz.
    explicit SampleBuffer(SampleRate aSampleRate)
      : mSampleRate(aSampleRate)

    {
    }

    /// @brief Get the sample rate.
    /// @return Sample rate in Hz.
    [[nodiscard]] SampleRate GetSampleRate() const { return mSampleRate; }

    /// @brief Get the total number of samples stored.
    /// @return Number of samples.
    [[nodiscard]] SampleCount GetSampleCount() const;

    /// @brief Add audio samples to buffer.
    /// @param samples Vector of samples to append.
    void AddSamples(const std::vector<float>& aSamples);

    /// @brief Get samples from the buffer.
    /// @param aStartSample Starting sample index
    /// @param aSampleCount Number of samples to retrieve.
    /// @return Read-only span of samples.
    /// @throws std::out_of_range if there aren't enough samples to fill the request.
    [[nodiscard]] std::span<const float> GetSamples(SampleIndex aStartSample,
                                                    SampleCount aSampleCount) const;

  private:
    SampleRate mSampleRate;
    std::vector<float> mData;
};
