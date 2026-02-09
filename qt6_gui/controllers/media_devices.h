// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include "controllers/audio_device.h"
#include <QAudioDevice>
#include <QAudioFormat>
#include <QByteArray>
#include <QMediaDevices>
#include <QString>
#include <memory>
#include <vector>

/// @brief Interface for audio device enumeration
///
/// Pure virtual interface for enumerating audio input devices.
/// This allows mocking QMediaDevices for unit testing.
class IMediaDevices
{
  public:
    virtual ~IMediaDevices() = default;

    /// @brief Get all available audio input devices
    /// @return List of audio devices
    [[nodiscard]] virtual std::vector<std::unique_ptr<IAudioDevice>> AudioInputs() const = 0;

    /// @brief Get the default audio input device
    /// @return Default audio device
    [[nodiscard]] virtual std::unique_ptr<IAudioDevice> DefaultAudioInput() const = 0;

    /// @brief Find a device by its ID
    /// @param aDeviceId The device ID to search for
    /// @return Pointer to device if found, nullptr otherwise
    [[nodiscard]] virtual std::unique_ptr<IAudioDevice> GetAudioInputById(
      const QByteArray& aDeviceId) const = 0;
};

/// @brief Concrete implementation of IMediaDevices wrapping QMediaDevices
class MediaDevices : public IMediaDevices
{
  public:
    MediaDevices() = default;
    [[nodiscard]] std::vector<std::unique_ptr<IAudioDevice>> AudioInputs() const override;
    [[nodiscard]] std::unique_ptr<IAudioDevice> DefaultAudioInput() const override;
    [[nodiscard]] std::unique_ptr<IAudioDevice> GetAudioInputById(
      const QByteArray& aDeviceId) const override;
};
