// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once
#include <QObject>
#include <memory>
#include <sample_buffer.h>
#include <span>
#include <vector>

/// @brief Multi-channel audio buffer
///
/// Wraps multiple SampleBuffer instances (one per channel) and provides
/// Qt signal/slot integration for the MVC architecture.
class AudioBuffer : public QObject
{
    Q_OBJECT

  public:
    /// @brief Constructor
    /// @param aParent Qt parent object (optional)
    explicit AudioBuffer(QObject* aParent = nullptr);

    /// @brief Reset the audio buffer, clearing all samples
    /// @param aChannelCount New channel count
    /// @param aSampleRate New sample rate in Hz
    /// @throws std::invalid_argument if aChannelCount or aSampleRate is invalid
    /// @note Intended to be used when starting recording or loading a file.
    void Reset(ChannelCount aChannelCount, SampleRate aSampleRate);

    /// @brief Get the number of channels
    /// @return Channel count
    [[nodiscard]] ChannelCount GetChannelCount() const { return mChannelCount; }

    /// @brief Get the sample rate
    /// @return Sample rate in Hz
    [[nodiscard]] SampleRate GetSampleRate() const { return mSampleRate; }

    /// @brief Add interleaved audio samples to all channels
    /// @param aSamples Interleaved audio data (channel 0, channel 1, ..., repeat)
    /// @throws std::invalid_argument if sample count not divisible by channel count
    ///
    /// Example for stereo (2 channels): [L0, R0, L1, R1, L2, R2, ...]
    /// This method de-interleaves and appends to respective channel buffers.
    ///
    /// Emits dataAvailable() signal after samples are added.
    void AddSamples(const std::vector<float>& aSamples);

    /// @brief Get samples from a specific channel
    /// @param aChannelIndex Channel index (0-based)
    /// @param aStartSample Starting sample index
    /// @param aSampleCount Number of samples to retrieve
    /// @return Read-only span of samples.
    /// @throws std::out_of_range if aChannelIndex >= channel count, or if there
    /// aren't enough samples to fill the request.
    [[nodiscard]] std::span<const float> GetSamples(ChannelCount aChannelIndex,
                                                    SampleIndex aStartSample,
                                                    SampleCount aSampleCount) const;

    /// @brief Get the underlying SampleBuffer for a specific channel
    /// @param aChannelIndex Channel index (0-based)
    /// @return Reference to the SampleBuffer for the channel
    /// @throws std::out_of_range if aChannelIndex >= channel count
    [[nodiscard]] const SampleBuffer& GetChannelBuffer(ChannelCount aChannelIndex) const;

    /// @brief Get the total number of frames available
    /// @return Frame count
    [[nodiscard]] FrameCount GetFrameCount() const
    {
        if (mChannelBuffers.empty()) {
            return FrameCount(0);
        }
        // Cast to FrameCount.  In AudioBuffer, all channels have the same
        // sample count, so we can use any channel to get the frame count.
        return FrameCount(mChannelBuffers[0]->GetSampleCount().Get());
    }

  signals:
    /// @brief Emitted when new audio frames are added
    /// @param aTotalFrameCount Total number of frames available per channel
    void DataAvailable(FrameCount aTotalFrameCount);

    /// @brief Emitted when the buffer is reset
    ///
    /// This notifies listeners to clear any cached data.
    void BufferReset();

  private:
    /// @brief Initialize empty mChannelBuffers with given channel count and sample rate
    /// @param aChannelCount Number of audio channels
    /// @param aSampleRate Sample rate in Hz
    /// @throws std::invalid_argument if aChannelCount or aSampleRate is invalid
    /// @note This is a helper function used by the constructor and Reset() method.
    void InitializeChannelBuffers(ChannelCount aChannelCount, SampleRate aSampleRate);

    ChannelCount mChannelCount;
    SampleRate mSampleRate;
    std::vector<std::unique_ptr<SampleBuffer>> mChannelBuffers;
};
