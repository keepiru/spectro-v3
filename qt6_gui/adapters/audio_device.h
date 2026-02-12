
// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include <QAudioDevice>
#include <QAudioFormat>
#include <QByteArray>
#include <QString>
#include <utility>

/// @brief Interface for audio device information
///
/// Pure virtual interface for querying audio device properties.
/// This allows wrapping QAudioDevice for easier mocking in unit tests.
class IAudioDevice
{
  public:
    virtual ~IAudioDevice() = default;

    /// @brief Get the device unique identifier
    /// @return Device ID as QByteArray
    [[nodiscard]] virtual QByteArray Id() const = 0;

    /// @brief Get the human-readable device description
    /// @return Device description string
    [[nodiscard]] virtual QString Description() const = 0;

    /// @brief Check if the device supports a specific audio format
    /// @param aFormat The audio format to check
    /// @return True if format is supported
    [[nodiscard]] virtual bool IsFormatSupported(const QAudioFormat& aFormat) const = 0;

    /// @brief Get the underlying QAudioDevice
    /// @return The wrapped QAudioDevice
    [[nodiscard]] virtual QAudioDevice GetQAudioDevice() const = 0;
};

/// @brief Concrete implementation of IAudioDevice wrapping QAudioDevice
class AudioDevice : public IAudioDevice
{
  public:
    /// @brief Constructor
    /// @param aDevice The QAudioDevice to wrap
    explicit AudioDevice(QAudioDevice aDevice)
      : mDevice(std::move(aDevice))
    {
    }

    [[nodiscard]] QByteArray Id() const override { return mDevice.id(); }
    [[nodiscard]] QString Description() const override { return mDevice.description(); }
    [[nodiscard]] bool IsFormatSupported(const QAudioFormat& aFormat) const override
    {
        return mDevice.isFormatSupported(aFormat);
    }
    [[nodiscard]] QAudioDevice GetQAudioDevice() const override { return mDevice; }

  private:
    QAudioDevice mDevice;
};