#include "audio_buffer.h"
#include <cstdint>
#include <stdexcept>
#include <string>

AudioBuffer::AudioBuffer(size_t aChannelCount, size_t aSampleRate, QObject* aParent)
  : QObject(aParent)
  , mChannelCount(aChannelCount)
  , mSampleRate(aSampleRate)
  , mChannelBuffers(aChannelCount)
{
    if (aChannelCount == 0) {
        throw std::invalid_argument("AudioBuffer: Channel count must be > 0");
    }
    for (size_t i = 0; i < aChannelCount; ++i) {
        mChannelBuffers[i] = std::make_unique<SampleBuffer>(aSampleRate);
    }
}

size_t
AudioBuffer::GetNumSamples(size_t aChannelIndex) const
{
    if (aChannelIndex >= mChannelCount) {
        throw std::out_of_range("AudioBuffer::GetNumSamples: Channel index " +
                                std::to_string(aChannelIndex) + " out of range (0-" +
                                std::to_string(mChannelCount - 1) + ")");
    }
    return mChannelBuffers[aChannelIndex]->NumSamples();
}

void
AudioBuffer::AddSamples(const std::vector<float>& aSamples)
{
    if (aSamples.size() % mChannelCount != 0) {
        throw std::invalid_argument(
          "AudioBuffer::AddSamples: Sample count must be divisible by channel count");
    }

    const size_t kSamplesPerChannel = aSamples.size() / mChannelCount;

    // De-interleave samples into channel buffers
    for (size_t ch = 0; ch < mChannelCount; ++ch) {
        std::vector<float> channelSamples;
        channelSamples.reserve(kSamplesPerChannel);

        for (size_t i = 0; i < kSamplesPerChannel; ++i) {
            channelSamples.push_back(aSamples[i * mChannelCount + ch]);
        }

        mChannelBuffers[ch]->AddSamples(channelSamples);
    }

    emit dataAvailable(kSamplesPerChannel);
}

std::vector<float>
AudioBuffer::GetSamples(size_t aChannelIndex, int64_t aStartSample, size_t aSampleCount) const
{
    if (aChannelIndex >= mChannelCount) {
        throw std::out_of_range("AudioBuffer::GetSamples: Channel index " +
                                std::to_string(aChannelIndex) + " out of range (0-" +
                                std::to_string(mChannelCount - 1) + ")");
    }
    return mChannelBuffers[aChannelIndex]->GetSamples(aStartSample, aSampleCount);
}

SampleBuffer&
AudioBuffer::GetChannelBuffer(size_t aChannelIndex)
{
    if (aChannelIndex >= mChannelCount) {
        throw std::out_of_range("AudioBuffer::GetChannelBuffer: Channel index " +
                                std::to_string(aChannelIndex) + " out of range (0-" +
                                std::to_string(mChannelCount - 1) + ")");
    }
    return *mChannelBuffers[aChannelIndex];
}

const SampleBuffer&
AudioBuffer::GetChannelBuffer(size_t aChannelIndex) const
{
    if (aChannelIndex >= mChannelCount) {
        throw std::out_of_range("AudioBuffer::GetChannelBuffer: Channel index " +
                                std::to_string(aChannelIndex) + " out of range (0-" +
                                std::to_string(mChannelCount - 1) + ")");
    }
    return *mChannelBuffers[aChannelIndex];
}