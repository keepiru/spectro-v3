#include <cstddef>
#include <cstdint>
#include <format>
#include <mutex>
#include <sample_buffer.h>
#include <stdexcept>
#include <vector>

int64_t
SampleBuffer::NumSamples() const
{
    std::lock_guard<std::mutex> const kLock(mMutex);
    if (mData.size() > static_cast<size_t>(std::numeric_limits<int64_t>::max())) {
        throw std::overflow_error("SampleBuffer::NumSamples: sample count exceeds int64_t maximum");
    }
    return static_cast<int64_t>(mData.size());
}

void
SampleBuffer::AddSamples(const std::vector<float>& aSamples)
{
    std::lock_guard<std::mutex> const kLock(mMutex);
    mData.insert(mData.end(), aSamples.begin(), aSamples.end());
}

std::vector<float>
SampleBuffer::GetSamples(size_t aStartSample, size_t aSampleCount) const
{
    std::lock_guard<std::mutex> const kLock(mMutex);
    if (aStartSample > mData.size() || aSampleCount > (mData.size() - aStartSample)) {
        throw std::out_of_range(
          std::format("SampleBuffer::GetSamples: Not enough samples to fulfill request: "
                      "requested start {}, count {}, available {}",
                      aStartSample,
                      aSampleCount,
                      mData.size()));
    }

    // The check above guarantees the range is valid.
    // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
    return { mData.begin() + aStartSample, mData.begin() + aStartSample + aSampleCount };
}