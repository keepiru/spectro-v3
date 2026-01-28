// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "audio_file_reader.h"
#include "audio_types.h"
#include <format>
#include <sndfile.h>
#include <stdexcept>
#include <string>
#include <vector>

AudioFileReader::AudioFileReader(const std::string& aFilePath)
  : mSfInfo()
{
    // Open the audio file for reading
    mSndFile.reset(sf_open(aFilePath.c_str(), SFM_READ, &mSfInfo));
    if (!mSndFile) {
        const char* kSfError = sf_strerror(nullptr);
        throw std::runtime_error(
          std::format("Failed to open audio file {} for reading: {}", aFilePath, kSfError));
    }
}

std::vector<float>
AudioFileReader::ReadInterleaved(FrameCount aFrames)
{
    // Read interleaved audio samples from the file
    SampleCount const kTotalSamples = aFrames * ChannelCount(mSfInfo.channels);
    std::vector<float> buffer(kTotalSamples.Get());
    const sf_count_t framesRead =
      sf_readf_float(mSndFile.get(), buffer.data(), aFrames.ToSfCountT());
    buffer.resize(framesRead * mSfInfo.channels);
    return buffer;
}
