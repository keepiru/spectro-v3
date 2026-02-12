// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "adapters/media_devices.h"
#include "adapters/audio_device.h"
#include <QByteArray>
#include <QList>
#include <QMediaDevices>
#include <memory>
#include <vector>

std::vector<std::unique_ptr<IAudioDevice>>
MediaDevices::AudioInputs() const
{
    const QList<QAudioDevice> kQAudioDevices = QMediaDevices::audioInputs();
    std::vector<std::unique_ptr<IAudioDevice>> result;
    for (const QAudioDevice& kQAudioDevice : kQAudioDevices) {
        result.push_back(std::make_unique<AudioDevice>(kQAudioDevice));
    }
    return result;
}

std::unique_ptr<IAudioDevice>
MediaDevices::DefaultAudioInput() const
{
    return std::make_unique<AudioDevice>(QMediaDevices::defaultAudioInput());
}

std::unique_ptr<IAudioDevice>
MediaDevices::GetAudioInputById(const QByteArray& aDeviceId) const
{
    const QList<QAudioDevice> kDevices = QMediaDevices::audioInputs();
    for (const QAudioDevice& kDevice : kDevices) {
        if (kDevice.id() == aDeviceId) {
            return std::make_unique<AudioDevice>(kDevice);
        }
    }
    return nullptr;
}
