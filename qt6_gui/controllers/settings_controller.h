// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include "audio_types.h"
#include <QAudioDevice>
#include <QList>
#include <QObject>
#include <array>
#include <expected>
#include <string>
#include <vector>

// Forward declarations
class Settings;

/// @brief Controller for application settings
///
/// Coordinates the interaction between SettingsPanel (view) and Settings (model).
/// Handles audio device enumeration and capability queries.
class SettingsController : public QObject
{
    Q_OBJECT

  public:
    /// @brief Constructor
    /// @param aSettings Reference to the settings model
    /// @param aParent Qt parent object (optional)
    explicit SettingsController(Settings& aSettings, QObject* aParent = nullptr);

    /// @brief Get available audio input devices and default device
    /// @return AudioDeviceList containing available devices and default device ID
    [[nodiscard]] QList<QAudioDevice> GetAudioDevices() const;

    /// @brief Get the default audio input device
    /// @return The default QAudioDevice
    [[nodiscard]] QAudioDevice GetDefaultAudioInput() const;

    /// @brief Get supported sample rates for a specific audio device
    /// @param aDeviceId The device ID to query
    /// @return Vector of supported sample rates (empty if device not found)
    [[nodiscard]] std::expected<std::vector<SampleRate>, std::string> GetSupportedSampleRates(
      const QByteArray& aDeviceId) const;

    /// @brief Get supported channel counts for a specific audio device
    /// @param aDeviceId The device ID to query
    /// @return Vector of supported channel counts (empty if device not found)
    [[nodiscard]] std::expected<std::vector<ChannelCount>, std::string> GetSupportedChannels(
      const QByteArray& aDeviceId) const;

    /// @brief Find an audio device by its ID
    /// @param aDeviceId The device ID to search for
    /// @return The audio device if found, std::nullopt otherwise
    [[nodiscard]] std::expected<QAudioDevice, std::string> GetAudioDeviceById(
      const QByteArray& aDeviceId) const;

  private:
    Settings& mSettings;

    /// @brief Common sample rates to test for device support
    static constexpr std::array<SampleRate, 8> KValidSampleRates = { 8000,  11025, 16000, 22050,
                                                                     44100, 48000, 88200, 96000 };
    /// @brief Common input channel configurations: mono (1), stereo (2), quad (4), and 5.1-style
    /// 6-channel.
    static constexpr std::array<ChannelCount, 4> KValidChannelCounts = { 1, 2, 4, 6 };
};
