// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "audio_types.h"
#include <format>
#include <iterator>
#include <sample_buffer.h>
#include <stdexcept>
#include <vector>

SampleCount
SampleBuffer::GetSampleCount() const
{
    // NOLINTNEXTLINE(modernize-return-braced-init-list) - explicit constructor required
    return SampleCount(mData.size());
}

void
SampleBuffer::AddSamples(const std::vector<float>& aSamples)
{
    mData.insert(mData.end(), aSamples.begin(), aSamples.end());
}

std::vector<float>
SampleBuffer::GetSamples(SampleIndex aStartSample, SampleCount aSampleCount) const
{
    if (aStartSample.Get() > mData.size() ||
        aSampleCount.Get() > mData.size() - aStartSample.Get()) {
        throw std::out_of_range(
          std::format("SampleBuffer::GetSamples: Not enough samples to fulfill request: "
                      "requested start {}, count {}, available {}",
                      aStartSample.Get(),
                      aSampleCount.Get(),
                      mData.size()));
    }

    // The check above guarantees the range is valid.
    const auto kStartIt = std::next(mData.begin(), aStartSample.AsPtrDiffT());
    const auto kEndIt = std::next(kStartIt, aSampleCount.AsPtrDiffT());
    return { kStartIt, kEndIt };
}