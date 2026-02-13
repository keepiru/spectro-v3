// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "controllers/audio_player.h"
#include "adapters/audio_buffer_qiodevice.h"
#include "adapters/audio_sink.h"
#include "audio_types.h"
#include <QAudioFormat>
#include <QAudioSink>
#include <expected>
#include <memory>
#include <string>
#include <utility>

std::expected<void, std::string>
AudioPlayer::Start(FrameIndex aStartFrame)
{
    if (IsPlaying()) {
        Stop();
    }

    mAudioSink = mAudioSinkFactory();
    if (mAudioSink == nullptr) {
        return std::unexpected("Failed to create audio sink");
    }

    auto audioBufferQIODevice = std::make_unique<AudioBufferQIODevice>(mAudioBuffer);
    if (!audioBufferQIODevice->open(QIODevice::ReadOnly)) {
        mAudioSink.reset();
        return std::unexpected("Failed to open AudioBufferQIODevice");
    }

    if (!audioBufferQIODevice->SeekFrame(aStartFrame)) {
        mAudioSink.reset();
        return std::unexpected("Failed to seek to start frame in AudioBufferQIODevice");
    }

    mAudioSink->Start(std::move(audioBufferQIODevice));
    return {};
}

void
AudioPlayer::Stop()
{
    if (mAudioSink) {
        mAudioSink->Stop();
        mAudioSink.reset();
    }
}

AudioPlayer::AudioSinkFactory
AudioPlayer::DefaultAudioSinkFactory()
{
    return [this]() {
        const QAudioFormat kFormat = mAudioBuffer.GetAudioFormat();
        return std::make_unique<AudioSink>(kFormat);
    };
}