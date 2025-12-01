#include <audio_buffer.h>
#include <exception>

size_t
AudioBuffer::numFrames() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_data[0].size();
}

void
AudioBuffer::add_frames_planar(const std::vector<std::vector<float>>& samples)
{
    if (samples.size() != m_channels) {
        throw std::invalid_argument(
          "Number of channels in samples does not match AudioBuffer channels");
    }

    for (auto& channel_samples : samples) {
        if (channel_samples.size() != samples[0].size()) {
            throw std::invalid_argument("All channels must have the same number of frames");
        }
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    for (size_t ch = 0; ch < m_channels; ++ch) {
        m_data[ch].insert(m_data[ch].end(), samples[ch].begin(), samples[ch].end());
    }
}

std::vector<float>
AudioBuffer::get_channel_samples(size_t channel, size_t start_frame, size_t frame_count) const
{
    if (channel >= m_channels) {
        throw std::out_of_range("Channel index out of range");
    }
    if (start_frame + frame_count > m_data[channel].size()) {
        throw std::out_of_range("Requested frame range out of bounds");
    }
    return std::vector<float>(m_data[channel].begin() + start_frame,
                              m_data[channel].begin() + start_frame + frame_count);
}