// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "audio_recorder.h"
#include "audio_buffer.h"
#include "include/global_constants.h"
#include <QAudio>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSource>
#include <QByteArray>
#include <QIODevice>
#include <QObject>
#include <QtTypes>
#include <audio_types.h>
#include <cstddef>
#include <format>
#include <memory>
#include <stdexcept>
#include <vector>

AudioRecorder::AudioRecorder(AudioBuffer& aAudioBuffer, QObject* aParent)
  : QObject(aParent)
  , mAudioBuffer(aAudioBuffer)
{
}

bool
AudioRecorder::Start(const QAudioDevice& aQAudioDevice,
                     ChannelCount aChannelCount,
                     SampleRate aSampleRate,
                     QIODevice* aMockQIODevice)
{
    if (aChannelCount == 0 || aChannelCount > GKMaxChannels) {
        throw std::invalid_argument(
          std::format("{}: Invalid channel count {}", __PRETTY_FUNCTION__, aChannelCount));
    }

    if (aSampleRate <= 0) {
        throw std::invalid_argument(
          std::format("{}: Invalid sample rate {}", __PRETTY_FUNCTION__, aSampleRate));
    }

    QAudioFormat format;
    format.setSampleRate(aSampleRate);
    format.setChannelCount(aChannelCount);
    format.setSampleFormat(QAudioFormat::Float);

    mAudioBuffer.Reset(aChannelCount, aSampleRate);

    mAudioSource = std::make_unique<QAudioSource>(aQAudioDevice, format);

    if (!mAudioSource) {
        emit ErrorOccurred("Failed to create QAudioSource");
        return false;
    }

    // How many bytes the source should buffer before triggering readyRead.
    // 44100Hz sample rate * 2 channels * 4 bytes per sample / 60Hz display rate
    // gives us 5880 bytes.  We'll choose something smaller than that to keep
    // the updates coming quickly even if the sample rate is lower.  In
    // practice, we seem to get ~3-4K, perhaps due to scheduling limitations.
    constexpr qsizetype kSourceBufferSize = 2048;
    mAudioSource->setBufferSize(kSourceBufferSize);

    // Unfortunately we can't override start() to inject a mock QIODevice, so we
    // just assign it here for testing.
    if (aMockQIODevice) {
        mAudioIODevice = aMockQIODevice;
    } else {
        mAudioIODevice = mAudioSource->start();
    }

    if (!mAudioIODevice) {
        emit ErrorOccurred("Failed to start audio input");
        return false;
    }

    connect(mAudioIODevice, &QIODevice::readyRead, this, &AudioRecorder::ReadAudioData);
    emit RecordingStateChanged(true);
    return true;
}

void
AudioRecorder::Stop()
{
    if (mAudioSource) {
        mAudioSource->stop();
        mAudioSource.reset();
        emit RecordingStateChanged(false);
    }
    mAudioIODevice = nullptr;
}

void
AudioRecorder::ReadAudioData()
{
    if (!mAudioIODevice) {
        // This should be set during Start(), and this callback shouldn't
        // happen unless we're started and recording.
        throw std::runtime_error("AudioRecorder::ReadAudioData called when not recording");
    }

    // Read it into a QByteArray, then convert to float vector.
    const QByteArray audioData = mAudioIODevice->readAll();
    if (audioData.size() % sizeof(float) != 0) {
        throw std::runtime_error(
          "AudioRecorder::ReadAudioData: Read bytes not divisible by sample size");
    }

    const auto sampleCount = static_cast<size_t>(audioData.size() / sizeof(float));

    // Type punning is intentional: we need to interpret the byte stream as floats.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto* sampleData = reinterpret_cast<const float*>(audioData.constData());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const std::vector<float> samples(sampleData, sampleData + sampleCount);

    // Then send it to the AudioBuffer.
    mAudioBuffer.AddSamples(samples);
}

bool
AudioRecorder::IsRecording() const
{
    // What we really want is to check mAudioSource->state() ==
    // QAudio::ActiveState, but that doesn't work in test environments without
    // actual audio hardware.  Instead, we just check whether we have an active
    // QIODevice, which is a good proxy for whether we're recording.  This won't
    // track paused states (we don't use that), or external state changes (we
    // can cross that bridge when we get there).
    return mAudioIODevice != nullptr;
}
