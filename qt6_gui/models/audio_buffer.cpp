#include "audio_buffer.h"
#include <stdexcept>

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
AudioBuffer::GetChannelCount() const
{
    return mChannelCount;
}