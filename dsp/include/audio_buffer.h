#pragma once
#include <cstddef>
#include <mutex>
#include <vector>

/**
 * @brief A thread-safe audio sample storage.
 *
 * Stores multi-channel audio.  Supports random access for scrubbing.
 * Thread-safe for concurrent reads and writes.
 */
class AudioBuffer
{
  public:
    /**
     * @brief Construct an AudioBuffer.
     * @param channels Number of audio channels.
     * @param sample_rate Sample rate in Hz.
     */
    explicit AudioBuffer(size_t channels, size_t sample_rate)
      : m_channels(channels)
      , m_sample_rate(sample_rate)
      , m_data(channels)
    {
    }

    /**
     * @brief Get the number of channels.
     * @return Number of channels.
     */
    size_t channels() const { return m_channels; }

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
     * @param samples Vector of channels, each containing a vector of samples.
     * @throws std::invalid_argument if channel count does not match or if
     * channels have different frame counts.
     */
    void add_frames_planar(const std::vector<std::vector<float>>& samples);

    /**
     * @brief Get samples for a specific channel.
     * @param channel Channel index.
     * @param start_frame Starting frame index, possibly negative.
     * @param frame_count Number of frames to retrieve.
     * @return Vector of samples for the specified channel.  Invalid frames are
     * filled with zeros.
     * @throws std::out_of_range if channel index is invalid.
     */
    std::vector<float> get_channel_samples(size_t channel,
                                           int64_t start_frame,
                                           size_t frame_count) const;

  private:
    mutable std::mutex m_mutex;
    size_t m_channels;
    size_t m_sample_rate;
    std::vector<std::vector<float>> m_data;
};
