#include "audio_types.h"
#include <format>
#include <sample_buffer.h>
#include <stdexcept>
#include <vector>

SampleCount
SampleBuffer::GetSampleCount() const
{
    return mData.size();
}

void
SampleBuffer::AddSamples(const std::vector<float>& aSamples)
{
    mData.insert(mData.end(), aSamples.begin(), aSamples.end());
}

std::vector<float>
SampleBuffer::GetSamples(SampleIndex aStartSample, SampleCount aSampleCount) const
{
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