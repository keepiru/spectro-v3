// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "controllers/settings_controller.h"
#include "audio_types.h"
#include "controllers/audio_device.h"
#include "controllers/audio_recorder.h"
#include "controllers/media_devices.h"
#include "models/settings.h"
#include <QAudioFormat>
#include <QObject>
#include <memory>
#include <optional>
#include <vector>

SettingsController::SettingsController(Settings& aSettings,
                                       IMediaDevices& aAudioDeviceProvider,
                                       AudioRecorder& aRecorder,
                                       QObject* aParent)
  : QObject(aParent)
  , mSettings(aSettings)
  , mAudioDeviceProvider(aAudioDeviceProvider)
  , mRecorder(aRecorder)
{
}

std::optional<std::vector<SampleRate>>
SettingsController::GetSupportedSampleRates(const QByteArray& aDeviceId) const
{
    const auto kDevice = GetAudioInputById(aDeviceId);
    if (!kDevice) {
        return std::nullopt;
    }

    std::vector<SampleRate> result;

    // Test each common sample rate with a typical channel count (2 for stereo)
    for (const SampleRate kSampleRate : KValidSampleRates) {
        QAudioFormat format;
        format.setSampleRate(kSampleRate);
        format.setChannelCount(2);
        format.setSampleFormat(QAudioFormat::Float);

        if (kDevice->IsFormatSupported(format)) {
            result.push_back(kSampleRate);
        }
    }

    return result;
}

std::optional<std::vector<ChannelCount>>
SettingsController::GetSupportedChannels(const QByteArray& aDeviceId) const
{
    const auto kDevice = GetAudioInputById(aDeviceId);
    if (!kDevice) {
        return std::nullopt;
    }

    std::vector<ChannelCount> result;
    const SampleRate kTestSampleRate = 44100;

    // Test each channel count with a sample rate the device should support
    for (const ChannelCount kChannelCount : KValidChannelCounts) {
        QAudioFormat format;
        format.setSampleRate(kTestSampleRate);
        format.setChannelCount(kChannelCount);
        format.setSampleFormat(QAudioFormat::Float);

        if (kDevice->IsFormatSupported(format)) {
            result.push_back(kChannelCount);
        }
    }

    return result;
}

bool
SettingsController::StartRecording(const QByteArray& aDeviceId,
                                   ChannelCount aChannels,
                                   SampleRate aSampleRate)
{
    auto device = GetAudioInputById(aDeviceId);
    if (!device) {
        return false;
    }

    return mRecorder.Start(*device, aChannels, aSampleRate);
}
