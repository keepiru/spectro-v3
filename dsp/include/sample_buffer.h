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
    int64_t NumSamples() const;

    /**
     * @brief Add audio samples to buffer.
     * @param samples Vector of samples to append.
     */
    void AddSamples(const std::vector<float>& aSamples);

    /**
     * @brief Get samples from the buffer.
     * @param aStartSample Starting sample index
     * @param aSampleCount Number of samples to retrieve.
     * @return Vector of samples.
     * @throws std::out_of_range if there aren't enough samples to fill the request.
     */
    std::vector<float> GetSamples(size_t aStartSample, size_t aSampleCount) const;

  private:
    mutable std::mutex mMutex;
    size_t mSampleRate;
    std::vector<float> mData;
};
