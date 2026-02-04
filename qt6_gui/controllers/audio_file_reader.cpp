// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "audio_file_reader.h"
#include "audio_types.h"
#include <expected>
#include <format>
#include <sndfile.h>
#include <string>
#include <vector>

AudioFileReader::AudioFileReader(SNDFILE* aSndFile, SF_INFO aSfInfo)
  : mSfInfo(aSfInfo)
  , mSndFile(aSndFile, &sf_close)
{
}

std::expected<AudioFileReader, std::string>
AudioFileReader::Open(const std::string& aFilePath)
{
    SF_INFO sfInfo{};
    SNDFILE* sndFile = sf_open(aFilePath.c_str(), SFM_READ, &sfInfo);
    if (!sndFile) {
        const char* kSfError = sf_strerror(nullptr);
        return std::unexpected(
          std::format("Failed to open audio file {} for reading: {}", aFilePath, kSfError));
    }
    return AudioFileReader(sndFile, sfInfo);
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
