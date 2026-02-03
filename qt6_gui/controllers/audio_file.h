// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once
#include "controllers/audio_file_reader.h"
#include "models/audio_buffer.h"
#include <QObject>
#include <functional>
#include <string>

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

    /// @brief Convenience method to load an audio file from a file path
    /// @param aFilePath Path to the audio file
    /// @param aProgressCallback Callback for progress updates
    /// @return True if the file was loaded successfully, false otherwise
    bool LoadFile(const std::string& aFilePath, const ProgressCallback& aProgressCallback);

    /// @brief Load an audio file
    /// @param aReader Audio file reader interface
    /// @param aProgressCallback Callback for progress updates
    /// @return True if the file was loaded successfully, false otherwise
    bool LoadFileFromReader(IAudioFileReader& aReader, const ProgressCallback& aProgressCallback);

  private:
    AudioBuffer& mBuffer;
};
