// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include "controllers/audio_device.h"
#include "controllers/media_devices.h"
#include <QAudioDevice>
#include <QAudioFormat>
#include <QByteArray>
#include <QString>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

/// @brief Mock audio device for testing
class MockAudioDevice : public IAudioDevice
{

  public:
    using FormatSupportedFunc = std::function<bool(const QAudioFormat&)>;

    MockAudioDevice(
      QByteArray aId = "mock-device",
      QString aDescription = "Mock Audio Device",
      FormatSupportedFunc aFormatSupported = [](const QAudioFormat&) { return true; })
      : mId(std::move(aId))
      , mDescription(std::move(aDescription))
      , mFormatSupported(std::move(aFormatSupported))
    {
    }

    [[nodiscard]] QByteArray Id() const override { return mId; }
    [[nodiscard]] QString Description() const override { return mDescription; }
    [[nodiscard]] bool IsFormatSupported(const QAudioFormat& aFormat) const override
    {
        return mFormatSupported(aFormat);
    }
    [[nodiscard]] QAudioDevice GetQAudioDevice() const override { return {}; }

  private:
    QByteArray mId;
    QString mDescription;
    FormatSupportedFunc mFormatSupported;
};

/// @brief Mock MediaDevices for testing
class MockMediaDevices : public IMediaDevices
{
  public:
    [[nodiscard]] std::vector<std::unique_ptr<IAudioDevice>> AudioInputs() const override
    {
        std::vector<std::unique_ptr<IAudioDevice>> result;
        result.reserve(mDevices.size());
        for (const auto& device : mDevices) {
            result.push_back(std::make_unique<MockAudioDevice>(device));
        }
        return result;
    }

    [[nodiscard]] std::unique_ptr<IAudioDevice> DefaultAudioInput() const override
    {
        return std::make_unique<MockAudioDevice>();
    }

    [[nodiscard]] std::optional<std::unique_ptr<IAudioDevice>> GetAudioInputById(
      const QByteArray& aDeviceId) const override
    {
        for (const auto& device : mDevices) {
            if (device.Id() == aDeviceId) {
                return std::make_unique<MockAudioDevice>(device);
            }
        }
        return std::nullopt;
    }

    /// @brief Helper to add a device with optional format support function
    void AddDevice(MockAudioDevice device) { mDevices.push_back(std::move(device)); }

  private:
    std::vector<MockAudioDevice> mDevices;
};
