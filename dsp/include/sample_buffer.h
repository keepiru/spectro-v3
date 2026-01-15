#pragma once
#include "audio_types.h"
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
    SampleRate GetSampleRate() const { return mSampleRate; }

    /// @brief Get the total number of samples stored.
    /// @return Number of samples.
    [[nodiscard]] SampleCount GetSampleCount() const;

    /// @brief Add audio samples to buffer.
    /// @param samples Vector of samples to append.
    void AddSamples(const std::vector<float>& aSamples);

    /// @brief Get samples from the buffer.
    /// @param aStartSample Starting sample index
    /// @param aSampleCount Number of samples to retrieve.
    /// @return Vector of samples.
    /// @throws std::out_of_range if there aren't enough samples to fill the request.
    [[nodiscard]] std::vector<float> GetSamples(SampleIndex aStartSample,
                                                SampleCount aSampleCount) const;

  private:
    SampleRate mSampleRate;
    std::vector<float> mData;
};
