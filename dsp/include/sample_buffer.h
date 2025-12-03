#pragma once
#include <cstddef>
#include <mutex>
#include <vector>

/**
 * @brief A thread-safe audio sample storage.
 *
 * Stores single-channel audio.  Supports random access for scrubbing.
 * Thread-safe for concurrent reads and writes.
 */
class SampleBuffer
{
  public:
    /**
     * @brief Construct a SampleBuffer.
     * @param aSampleRate Sample rate in Hz.
     */
    explicit SampleBuffer(size_t aSampleRate)
      : mSampleRate(aSampleRate)

    {
    }

    /**
     * @brief Get the sample rate.
     * @return Sample rate in Hz.
     */
    size_t GetSampleRate() const { return mSampleRate; }

    /**
     * @brief Get the total number of samples stored.
     * @return Number of samples.
     */
    size_t NumSamples() const;

    /**
     * @brief Add audio samples to buffer.
     * @param samples Vector of samples to append.
     */
    void AddSamples(const std::vector<float>& aSamples);

    /**
     * @brief Get samples from the buffer.
     * @param aStartSample Starting sample index, possibly negative.
     * @param aSampleCount Number of samples to retrieve.
     * @return Vector of samples.  Invalid samples are filled with zeros.
     */
    std::vector<float> GetSamples(int64_t aStartSample, size_t aSampleCount) const;

  private:
    mutable std::mutex mMutex;
    size_t mSampleRate;
    std::vector<float> mData;
};
