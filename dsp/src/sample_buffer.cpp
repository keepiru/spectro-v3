#include <cstddef>
#include <cstdint>
#include <mutex>
#include <sample_buffer.h>
#include <vector>

size_t
SampleBuffer::NumSamples() const
{
    std::lock_guard<std::mutex> const kLock(mMutex);
    return mData.size();
}

void
SampleBuffer::AddSamples(const std::vector<float>& aSamples)
{
    std::lock_guard<std::mutex> const kLock(mMutex);
    mData.insert(mData.end(), aSamples.begin(), aSamples.end());
}

std::vector<float>
SampleBuffer::GetSamples(int64_t aStartSample, size_t aSampleCount) const
{
    std::vector<float> result(aSampleCount, 0.0f);
    std::lock_guard<std::mutex> const kLock(mMutex);

    for (size_t i = 0; i < aSampleCount; ++i) {
        int64_t const kSampleIndex = aStartSample + static_cast<int64_t>(i);
        // Only copy from available data.  Everything else remains zeroed.
        if (kSampleIndex >= 0 && static_cast<size_t>(kSampleIndex) < mData.size()) {
            result[i] = mData[kSampleIndex];
        }
    }
    return result;
}