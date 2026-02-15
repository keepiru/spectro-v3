// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include "adapters/audio_sink.h"
#include "audio_types.h"
#include "models/audio_buffer.h"
#include <QAudioSink>
#include <QObject>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

/// @brief Plays audio from an AudioBuffer using QAudioSink.
/// This implements the simple case of just playing the buffer from start to finish.
class AudioPlayer : public QObject
{
    Q_OBJECT

  public:
    using AudioSinkFactory = std::function<std::unique_ptr<IAudioSink>()>;

    /// @brief Constructor.
    /// @param aAudioBuffer The AudioBuffer to play.
    /// @param aAudioSinkFactory Optional factory function to create IAudioSink
    /// instances for testing.  Production uses the default.
    /// @param aParent The parent QObject, if any.
    explicit AudioPlayer(AudioBuffer& aAudioBuffer,
                         AudioSinkFactory aAudioSinkFactory = nullptr,
                         QObject* aParent = nullptr)
      : QObject(aParent)
      , mAudioBuffer(aAudioBuffer)
      , mAudioSinkFactory(aAudioSinkFactory ? std::move(aAudioSinkFactory)
                                            : DefaultAudioSinkFactory())
    {
    }

    /// @brief Start playing the audio from the specified start frame.
    /// @param aStartFrame The frame index to start playing from.
    /// @return std::expected<void, std::string> indicating success or error message.
    [[nodiscard]] std::expected<void, std::string> Start(FrameIndex aStartFrame);

    /// @brief Stop playback if it is currently active.
    void Stop();

    /// @brief Check if currently playing.
    /// @return true if playback is active.
    [[nodiscard]] bool IsPlaying() const { return mAudioSink != nullptr; }

    /// @brief Get the current frame being played back
    /// @return FrameIndex of the current playback position, or std::nullopt if not playing
    [[nodiscard]] std::optional<FrameIndex> CurrentFrame() const;

  private:
    /// @brief Create an AudioSink for the system's default audio output device.
    /// @return A factory function that creates an AudioSink with the buffer's format.
    /// @note This implementation is used in production.
    AudioSinkFactory DefaultAudioSinkFactory();

    // The frame index where playback started.  This is used as a reference
    // point to calculate the current frame based on the elapsed time from the
    // audio sink.
    FrameIndex mStartFrame{ 0 };

    AudioBuffer& mAudioBuffer;
    AudioSinkFactory mAudioSinkFactory;
    std::unique_ptr<IAudioSink> mAudioSink;
};