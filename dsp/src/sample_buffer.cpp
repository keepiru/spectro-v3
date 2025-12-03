#include <cstddef>
#include <cstdint>
#include <mutex>
#include <sample_buffer.h>
#include <vector>

size_t
SampleBuffer::numSamples() const
{
    std::lock_guard<std::mutex> const lock(m_mutex);
    return m_data.size();
}

void
SampleBuffer::addSamples(const std::vector<float>& samples)
{
    std::lock_guard<std::mutex> const lock(m_mutex);
    m_data.insert(m_data.end(), samples.begin(), samples.end());
}

std::vector<float>
SampleBuffer::getSamples(int64_t start_sample, size_t sample_count) const
{
    std::vector<float> result(sample_count, 0.0f);
    std::lock_guard<std::mutex> const lock(m_mutex);

    for (size_t i = 0; i < sample_count; ++i) {
        int64_t const sample_index = start_sample + static_cast<int64_t>(i);
        // Only copy from available data.  Everything else remains zeroed.
        if (sample_index >= 0 && static_cast<size_t>(sample_index) < m_data.size()) {
            result[i] = m_data[sample_index];
        }
    }
    return result;
}