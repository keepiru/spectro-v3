#include <audio_buffer.h>
#include <exception>

size_t
AudioBuffer::numFrames() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_data.size();
}

void
AudioBuffer::add_frames(const std::vector<float>& samples)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data.insert(m_data.end(), samples.begin(), samples.end());
}

std::vector<float>
AudioBuffer::get_samples(int64_t start_frame, size_t frame_count) const
{
    std::vector<float> result(frame_count, 0.0f);
    std::lock_guard<std::mutex> lock(m_mutex);

    for (size_t i = 0; i < frame_count; ++i) {
        int64_t frame_index = start_frame + static_cast<int64_t>(i);
        // Only copy from available data.  Everything else remains zeroed.
        if (frame_index >= 0 && static_cast<size_t>(frame_index) < m_data.size()) {
            result[i] = m_data[frame_index];
        }
    }
    return result;
}