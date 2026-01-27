// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "audio_buffer.h"
#include "include/global_constants.h"
#include <QObject>
#include <audio_types.h>
#include <cstddef>
#include <format>
#include <memory>
#include <sample_buffer.h>
#include <span>
#include <stdexcept>
#include <vector>

AudioBuffer::AudioBuffer(QObject* aParent)
  : QObject(aParent)
{
    constexpr ChannelCount kDefaultChannelCount = 2;
    constexpr SampleRate kDefaultSampleRate = 44100;
    InitializeChannelBuffers(kDefaultChannelCount, kDefaultSampleRate);
}

void
AudioBuffer::InitializeChannelBuffers(ChannelCount aChannelCount, SampleRate aSampleRate)
{
    if (aChannelCount == 0) {
        throw std::invalid_argument(
          std::format("{}: Channel count must be > 0", __PRETTY_FUNCTION__));
    }

    if (aChannelCount > GKMaxChannels) {
        throw std::invalid_argument(
          std::format("{}: Channel count exceeds maximum supported channels", __PRETTY_FUNCTION__));
    }

    if (aSampleRate <= 0) {
        throw std::invalid_argument(
          std::format("{}: Sample rate must be > 0", __PRETTY_FUNCTION__));
    }

    mChannelCount = aChannelCount;
    mSampleRate = aSampleRate;

    // This is called infrequently.  It's not worth optimizing to avoid some
    // small reallocations.  Just wipe the vector for the sake of simplicity and
    // correctness.
    mChannelBuffers.clear();
    mChannelBuffers.resize(aChannelCount);

    for (size_t i = 0; i < aChannelCount; ++i) {
        mChannelBuffers[i] = std::make_unique<SampleBuffer>(aSampleRate);
    }
}

void
AudioBuffer::Reset(ChannelCount aChannelCount, SampleRate aSampleRate)
{
    InitializeChannelBuffers(aChannelCount, aSampleRate);

    // Invalidate any cached data in listeners
    emit BufferReset();
}

void
AudioBuffer::AddSamples(const std::vector<float>& aSamples)
{
    if (aSamples.size() % mChannelCount != 0) {
        throw std::invalid_argument(
          "AudioBuffer::AddSamples: Sample count must be divisible by channel count");
    }

    const size_t kSamplesPerChannel = aSamples.size() / mChannelCount;
    std::vector<float> channelSamples(kSamplesPerChannel); // Deinterleave buffer

    for (size_t channelID = 0; channelID < mChannelCount; channelID++) {
        // De-interleave one channel
        for (size_t i = 0; i < kSamplesPerChannel; i++) {
            channelSamples[i] = aSamples[(i * mChannelCount) + channelID];
        }

        // Then feed it to the SampleBuffer
        mChannelBuffers[channelID]->AddSamples(channelSamples);
    }

    emit DataAvailable(GetFrameCount());
}

std::span<const float>
AudioBuffer::GetSamples(const ChannelCount aChannelIndex,
                        const SampleIndex aStartSample,
                        const SampleCount aSampleCount) const
{
    if (aChannelIndex >= mChannelCount) {
        throw std::out_of_range("AudioBuffer::GetSamples: Channel index out of range");
    }

    return mChannelBuffers[aChannelIndex]->GetSamples(aStartSample, aSampleCount);
}

const SampleBuffer&
AudioBuffer::GetChannelBuffer(ChannelCount aChannelIndex) const
{
    if (aChannelIndex >= mChannelCount) {
        throw std::out_of_range("AudioBuffer::GetChannelBuffer: Channel index out of range");
    }

    return *mChannelBuffers[aChannelIndex];
}
