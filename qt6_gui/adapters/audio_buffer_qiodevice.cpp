// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "adapters/audio_buffer_qiodevice.h"
#include "audio_types.h"
#include "include/global_constants.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

bool
AudioBufferQIODevice::open(OpenMode aMode)
{
    if (aMode != QIODevice::ReadOnly) {
        return false;
    }
    mCurrentReadPosition = FrameIndex{ 0 };
    return QIODevice::open(aMode);
}

bool
AudioBufferQIODevice::SeekFrame(FrameIndex aFrame)
{
    const ChannelCount kChannelCount = mAudioBuffer.GetChannelCount();

    // If there are no channels, we can't seek anywhere.  This also prevents
    // division by zero below.  This shouldn't happen because AudioBuffer
    // doesn't allow 0 channels, but we'll handle it gracefully just in case.
    if (kChannelCount == 0) {
        return false;
    }

    const FrameCount kAvailableFrames{ mAudioBuffer.GetFrameCount() };

    if (aFrame.Get() > kAvailableFrames.Get()) {
        return false;
    }

    const qint64 kPositionBytes =
      static_cast<qint64>(aFrame.Get()) * mAudioBuffer.GetBytesPerFrame();
    if (!QIODevice::seek(kPositionBytes)) {
        return false;
    }

    mCurrentReadPosition = aFrame;
    return true;
}

qint64
AudioBufferQIODevice::readData(char* aData, qint64 aRequestedBytes)
{
    if (aRequestedBytes < 0) {
        return -1;
    }

    const ChannelCount kChannelCount = mAudioBuffer.GetChannelCount();

    // If there are no channels, we can't read any data.  This also prevents
    // division by zero below.  This shouldn't happen because AudioBuffer
    // doesn't allow 0 channels, but we'll handle it gracefully just in case.
    if (kChannelCount == 0) {
        return -1;
    }

    const BytesPerFrame kBytesPerFrame = mAudioBuffer.GetBytesPerFrame();
    const FrameCount kRequestedFrames{ static_cast<size_t>(aRequestedBytes / kBytesPerFrame) };
    const FrameCount kAvailableFrames = mAudioBuffer.GetFrameCount();

    // This might happen if the buffer was reset.
    if (mCurrentReadPosition.Get() > kAvailableFrames.Get()) {
        return -1;
    }

    const FrameCount kFramesRemaining{ kAvailableFrames.Get() - mCurrentReadPosition.Get() };
    const FrameCount kFramesToRead = std::min(kRequestedFrames, kFramesRemaining);

    // Interleave samples into output buffer
    for (ChannelCount ch = 0; ch < kChannelCount; ch++) {
        const SampleIndex kStartSample{ mCurrentReadPosition.Get() };
        const SampleCount kSamplesToRead{ kFramesToRead.Get() };
        const auto kSamples = mAudioBuffer.GetSamples(ch, kStartSample, kSamplesToRead);

        // The Qt API only provides us a pointer to a byte buffer, so we have to
        // copy the float samples as raw byte data.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        const char* kSamplesAsBytes = reinterpret_cast<const char*>(kSamples.data());

        for (size_t i = 0; i < kSamples.size(); i++) {
            const size_t kSourceOffset = i * sizeof(float);
            const size_t kDestOffset = (i * kChannelCount + ch) * sizeof(float);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            std::copy_n(kSamplesAsBytes + kSourceOffset, sizeof(float), aData + kDestOffset);
        }
    }

    mCurrentReadPosition = mCurrentReadPosition + kFramesToRead;

    return static_cast<int64_t>(kFramesToRead.Get() * kBytesPerFrame);
}

bool
AudioBufferQIODevice::seek(qint64 aPos)
{
    if (aPos != 0) {
        throw std::logic_error("AudioBufferQIODevice only supports seeking to the beginning of "
                               "the stream.  Use SeekFrame() to seek to specific frames.");
    }
    return SeekFrame(FrameIndex{ 0 });
}