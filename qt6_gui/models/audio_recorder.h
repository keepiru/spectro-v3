// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include "audio_types.h"
#include <QAudioFormat>
#include <QAudioSource>
#include <QIODevice>
#include <QObject>
#include <functional>
#include <memory>

class QAudioDevice;
class AudioBuffer;

/// @brief Captures audio from an input device and writes to an AudioBuffer.
///
/// Uses Qt Multimedia's QAudioSource to capture audio samples from
/// microphone/line-in. Supports runtime device changes and dependency injection
/// for testing. Audio format (sample rate, channels) is inferred from
/// AudioBuffer at Start().
class AudioRecorder : public QObject
{
    Q_OBJECT

  public:
    struct AudioSourceFactoryResult
    {
        std::unique_ptr<QAudioSource> audio_source;
        QIODevice* io_device = nullptr;
    };

    /// Factory function type for creating QAudioSource instances (for testing).
    /// We can't easily mock QAudioSource#start because it's not virtual, so we
    /// also start the QAudioSource here and return the QIODevice.
    using AudioSourceFactory =
      std::function<AudioSourceFactoryResult(const QAudioDevice&, const QAudioFormat&)>;

    /// @brief Constructs an AudioRecorder.
    /// @param aAudioBuffer The AudioBuffer to write captured samples to.
    /// @param aParent Qt parent object for memory management.
    explicit AudioRecorder(AudioBuffer& aAudioBuffer, QObject* aParent = nullptr);

    ~AudioRecorder() override;

    /// @brief Starts audio capture, writing samples to the specified buffer.
    /// @param aQAudioDevice The audio input device to capture from.
    /// @param aChannelCount Number of audio channels (e.g., 1 for mono, 2 for stereo).
    /// @param aSampleRate Sample rate in Hz (e.g., 44100, 48000).
    /// @param aMockQIODevice Mock audio IO device for testing.  (optional)
    /// @return true if capture started successfully, false otherwise.
    bool Start(const QAudioDevice& aQAudioDevice,
               ChannelCount aChannelCount,
               SampleRate aSampleRate,
               QIODevice* aMockQIODevice = nullptr);

    /// @brief Stops audio capture.
    /// @note no-op unless a capture is in progress.
    void Stop();

    /// @brief Returns whether audio capture is currently active.
    /// @return true if recording, false otherwise.
    [[nodiscard]] bool IsRecording() const;

  signals:
    /// @brief Emitted when recording state changes.
    /// @param aIsRecording true if now recording, false if stopped.
    void RecordingStateChanged(bool aIsRecording);

    /// @brief Emitted when an error occurs during capture.
    /// @param aErrorMessage Description of the error.
    void ErrorOccurred(const QString& aErrorMessage);

  private:
    std::unique_ptr<QAudioSource> mAudioSource;
    QIODevice* mAudioIODevice = nullptr;
    AudioBuffer& mAudioBuffer;

    /// @brief Reads available audio data and writes to the aBuffer.
    void ReadAudioData();
};
