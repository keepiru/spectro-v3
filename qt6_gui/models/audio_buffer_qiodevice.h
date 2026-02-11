// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include "audio_types.h"
#include "models/audio_buffer.h"
#include <QIODevice>
#include <QObject>

/// @brief A QIODevice wrapper around AudioBuffer to allow it to be used as a
/// source for QAudioSink.
/// @note The QIODevice abstraction is based on byte streams, so this class
/// handles converting between byte offsets and frame/sample indices, as well as
/// interleaving samples from multiple channels into a single stream of
/// interleaved audio data.
class AudioBufferQIODevice : public QIODevice
{
    Q_OBJECT

  public:
    /// @brief Constructor
    /// @param aAudioBuffer The AudioBuffer to read from.
    AudioBufferQIODevice(AudioBuffer& aAudioBuffer, QObject* aParent = nullptr)
      : QIODevice(aParent)
      , mAudioBuffer(aAudioBuffer)
    {
    }

    /// @brief Open the device.
    /// @param aMode Open mode.  Only QIODevice::ReadOnly is supported.
    [[nodiscard]] bool open(OpenMode aMode) override;

    /// @brief Seek to a specific frame in the audio stream.
    /// @param aFrame The frame index to seek to.
    /// @return true if the seek was successful, false if the position is out of
    /// range or if the device is not open.
    [[nodiscard]] bool SeekFrame(FrameIndex aFrame);

    /// @brief Partial implementation of QIODevice::seek()
    /// @param aPos The byte position to seek to.  Must be 0.
    /// @return true if the seek was successful, false otherwise.
    /// @throws std::logic_error if aPos is not 0
    /// @note Seeking to 0 is required for ::close() to work correctly.  Seeking
    /// to arbitrary byte positions would add a lot of complexity to support
    /// returning partial frames, which is not a use case we need.  Instead, we
    /// only support seeking to whole frames via SeekFrame().
    [[nodiscard]] bool seek(qint64 aPos) override;

  protected:
    /// @brief Reads data from the AudioBuffer into the provided buffer.
    /// @param aData Pointer to the buffer where interleaved audio data should be written.
    /// @param aRequestedBytes The number of bytes requested to read. If it is
    /// not a multiple of the frame size, it will be rounded down to the nearest
    /// whole frame.
    /// @return The number of bytes actually read, which may be less than
    /// requested if the end of the AudioBuffer is reached.
    [[nodiscard]] qint64 readData(char* aData, qint64 aRequestedBytes) override;

    /// @brief writeData is required for the QIODevice interface
    /// @note Not implemented because this adapter is read-only.
    [[nodiscard]] qint64 writeData(const char* /*aData*/, qint64 /*aMaxSize*/) override
    {
        return -1;
    }

  private:
    AudioBuffer& mAudioBuffer;
    FrameIndex mCurrentReadPosition{ 0 };
};
