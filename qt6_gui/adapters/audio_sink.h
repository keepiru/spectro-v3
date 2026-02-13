// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>
#include <memory>
#include <utility>

/// @brief Abstraction for QAudioSink to allow for easier mocking in unit tests.
class IAudioSink
{
  public:
    virtual ~IAudioSink() = default;

    /// @brief Start audio playback from the given source device
    /// @param aSourceQIODevice The source QIODevice to read audio from.
    virtual void Start(std::unique_ptr<QIODevice> aSourceQIODevice) = 0;

    /// @brief Stop audio playback
    virtual void Stop() = 0;
};

/// @brief Concrete implementation of IAudioSink wrapping QAudioSink
// LCOV_EXCL_START // This class exists to encapsulate untestable interfaces.
class AudioSink : public IAudioSink
{
  public:
    /// @brief Constructor
    /// @param aFormat The audio format for this sink
    explicit AudioSink(QAudioFormat aFormat)
      : mAudioSink(QAudioSink(aFormat))
    {
    }

    /// @brief Destructor
    /// Ensures that audio playback is stopped and resources are released.
    ~AudioSink() noexcept override { Stop(); }

    // Delete move and copy to prevent dangling pointer issues
    AudioSink(const AudioSink&) = delete;
    AudioSink& operator=(const AudioSink&) = delete;
    AudioSink(AudioSink&&) = delete;
    AudioSink& operator=(AudioSink&&) = delete;

    void Start(std::unique_ptr<QIODevice> aSourceQIODevice) noexcept override
    {
        Stop();
        mSourceQIODevice = std::move(aSourceQIODevice);
        mAudioSink.start(mSourceQIODevice.get());
    }

    void Stop() noexcept override
    {
        mAudioSink.stop();
        mSourceQIODevice.reset();
    }

  private:
    std::unique_ptr<QIODevice> mSourceQIODevice;
    QAudioSink mAudioSink;
};
// LCOV_EXCL_STOP