#include "audio_types.h"
#include <format>
#include <mutex>
#include <sample_buffer.h>
#include <stdexcept>
#include <vector>

SampleCount
SampleBuffer::GetSampleCount() const
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
SampleBuffer::GetSamples(SampleIndex aStartSample, SampleCount aSampleCount) const
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