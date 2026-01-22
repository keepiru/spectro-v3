// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once
#include "models/audio_buffer.h"
#include "models/audio_file_reader.h"
#include <QObject>
#include <QString>

/// @brief Controller for audio file operations
///
/// Handles loading and saving audio files, including format detection
/// and file metadata management.
class AudioFile : public QObject
{
    Q_OBJECT

  public:
    /// @brief Progress callback type
    /// @param aProgressPercent Progress percentage (0-100)
    using ProgressCallback = std::function<void(int aProgressPercent)>;

    /// @brief Constructor
    /// @param aParent Qt parent object (optional)
    explicit AudioFile(AudioBuffer& aBuffer, QObject* aParent = nullptr)
      : QObject(aParent)
      , mBuffer(aBuffer)
    {
    }

    /// @brief Load an audio file
    /// @param aReader Audio file reader interface
    /// @param aProgressCallback Callback for progress updates
    /// @return True if the file was loaded successfully, false otherwise
    bool LoadFile(IAudioFileReader& aReader, const ProgressCallback& aProgressCallback);

  private:
    AudioBuffer& mBuffer;
};
