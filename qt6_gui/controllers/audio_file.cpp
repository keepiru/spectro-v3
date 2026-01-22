// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "controllers/audio_file.h"
#include "models/audio_file_reader.h"
#include <audio_types.h>
#include <vector>

bool
AudioFile::LoadFile(IAudioFileReader& aReader, const ProgressCallback& aProgressCallback)
{
    // Each chunk is passed to AddSamples, which will trigger a display refresh,
    // so we want to keep these chunks fairly large for efficiency.
    constexpr FrameCount kChunkSize(1024L * 1024L);

    const ChannelCount kChannelCount = aReader.GetChannelCount();
    const SampleRate kSampleRate = aReader.GetSampleRate();
    const FrameCount kTotalFrames = aReader.GetFrameCount();
    int lastProgress = 0;

    mBuffer.Reset(kChannelCount, kSampleRate);

    while (true) {
        const std::vector<float> samples = aReader.ReadInterleaved(kChunkSize);
        if (samples.empty()) {
            break;
        }
        mBuffer.AddSamples(samples);

        // Report progress

        // Avoid division by zero for empty files.  This should never actually
        // happen because we would have exited the loop above, but let's be
        // safe and make sure we don't call the progress callback with Inf.
        if (kTotalFrames == FrameCount(0)) {
            continue;
        }
        const int kProgressPercent =
          static_cast<int>(static_cast<float>(mBuffer.GetFrameCount().Get()) /
                           static_cast<float>(kTotalFrames.Get()) * 100.0f);

        // Throttle progress callbacks to only report when it changes.
        if (kProgressPercent > lastProgress) {
            lastProgress = kProgressPercent;
            aProgressCallback(kProgressPercent);
        }
    }

    // Ensure we report 100% progress at the end.  Normally this only happens if
    // the file is empty.
    constexpr int kFinalProgressPercent = 100;
    if (lastProgress < kFinalProgressPercent) {
        aProgressCallback(kFinalProgressPercent);
    }

    return true;
}