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
     * @brief Get the total number of frames stored.
     * @return Number of frames.
     */
    size_t numFrames() const;

    /**
     * @brief Add audio frames to buffer.
     * @param samples Vector of samples to append.
     */
    void add_frames(const std::vector<float>& samples);

    /**
     * @brief Get samples from the buffer.
     * @param start_frame Starting frame index, possibly negative.
     * @param frame_count Number of frames to retrieve.
     * @return Vector of samples.  Invalid frames are filled with zeros.
     */
    std::vector<float> get_samples(int64_t start_frame, size_t frame_count) const;

  private:
    mutable std::mutex m_mutex;
    size_t m_sample_rate;
    std::vector<float> m_data;
};
