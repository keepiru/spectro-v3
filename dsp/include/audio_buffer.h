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
class AudioBuffer
{
  public:
    /**
     * @brief Construct an AudioBuffer.
     * @param sample_rate Sample rate in Hz.
     */
    explicit AudioBuffer(size_t sample_rate)
      : m_sample_rate(sample_rate)
      , m_data()
    {
    }

    /**
     * @brief Get the sample rate.
     * @return Sample rate in Hz.
     */
    size_t sample_rate() const { return m_sample_rate; }

    /**
     * @brief Get the total number of samples stored.
     * @return Number of samples.
     */
    size_t numSamples() const;

    /**
     * @brief Add audio samples to buffer.
     * @param samples Vector of samples to append.
     */
    void add_samples(const std::vector<float>& samples);

    /**
     * @brief Get samples from the buffer.
     * @param start_sample Starting sample index, possibly negative.
     * @param sample_count Number of samples to retrieve.
     * @return Vector of samples.  Invalid samples are filled with zeros.
     */
    std::vector<float> get_samples(int64_t start_sample, size_t sample_count) const;

  private:
    mutable std::mutex m_mutex;
    size_t m_sample_rate;
    std::vector<float> m_data;
};
