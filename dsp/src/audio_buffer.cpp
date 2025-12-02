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
AudioBuffer::get_channel_samples(size_t channel, int64_t start_frame, size_t frame_count) const
{
    if (channel >= m_channels) {
        throw std::out_of_range("Channel index out of range");
    }

    std::vector<float> result(frame_count, 0.0f);
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto& channel_data = m_data[channel];
    for (size_t i = 0; i < frame_count; ++i) {
        int64_t frame_index = start_frame + static_cast<int64_t>(i);
        // Only copy from available data.  Everything else remains zeroed.
        if (frame_index >= 0 && static_cast<size_t>(frame_index) < channel_data.size()) {
            result[i] = channel_data[frame_index];
        }
    }
    return result;
}