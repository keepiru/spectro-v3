// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "controllers/settings_controller.h"
#include "audio_types.h"
#include "models/settings.h"
#include <QAudioFormat>
#include <QList>
#include <QMediaDevices>
#include <QObject>
#include <expected>
#include <string>
#include <vector>

SettingsController::SettingsController(Settings& aSettings, QObject* aParent)
  : QObject(aParent)
  , mSettings(aSettings)
{
}

QList<QAudioDevice>
SettingsController::GetAudioDevices() const
{
    return QMediaDevices::audioInputs();
}

QAudioDevice
SettingsController::GetDefaultAudioInput() const
{
    return QMediaDevices::defaultAudioInput();
}

std::expected<std::vector<SampleRate>, std::string>
SettingsController::GetSupportedSampleRates(const QByteArray& aDeviceId) const
{
    const auto kDeviceOpt = GetAudioDeviceById(aDeviceId);
    if (!kDeviceOpt) {
        return std::unexpected(kDeviceOpt.error());
    }

    const auto& kDevice = kDeviceOpt.value();
    std::vector<SampleRate> result;

    // Test each common sample rate with a typical channel count (2 for stereo)
    for (const SampleRate kSampleRate : KValidSampleRates) {
        QAudioFormat format;
        format.setSampleRate(kSampleRate);
        format.setChannelCount(2);
        format.setSampleFormat(QAudioFormat::Float);

        if (kDevice.isFormatSupported(format)) {
            result.push_back(kSampleRate);
        }
    }

    return result;
}

std::expected<std::vector<ChannelCount>, std::string>
SettingsController::GetSupportedChannels(const QByteArray& aDeviceId) const
{
    const auto kDeviceOpt = GetAudioDeviceById(aDeviceId);
    if (!kDeviceOpt) {
        return std::unexpected(kDeviceOpt.error());
    }

    const auto& kDevice = kDeviceOpt.value();
    std::vector<ChannelCount> result;

    // We just need a sample rate that the device supports to test channel counts
    const auto kMinSampleRate = kDevice.minimumSampleRate();

    // Test each channel count
    for (const ChannelCount kChannelCount : KValidChannelCounts) {
        QAudioFormat format;
        format.setSampleRate(kMinSampleRate);
        format.setChannelCount(kChannelCount);
        format.setSampleFormat(QAudioFormat::Float);

        if (kDevice.isFormatSupported(format)) {
            result.push_back(kChannelCount);
        }
    }

    return result;
}

std::expected<QAudioDevice, std::string>
SettingsController::GetAudioDeviceById(const QByteArray& aDeviceId) const
{
    const auto kDevices = QMediaDevices::audioInputs();
    for (const auto& kDevice : kDevices) {
        if (kDevice.id() == aDeviceId) {
            return kDevice;
        }
    }
    return std::unexpected("Audio device not found");
}
