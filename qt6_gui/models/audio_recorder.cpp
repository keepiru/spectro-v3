#include "audio_recorder.h"

#include <QAudioDevice>

#include "audio_buffer.h"

AudioRecorder::AudioRecorder(QObject* parent)
  : QObject(parent)
{
}

AudioRecorder::~AudioRecorder()
{
    Stop();
}

bool
AudioRecorder::Start(AudioBuffer* buffer,
                     const QAudioDevice& device,
                     AudioSourceFactory aAudioSourceFactory)
{
    if (!buffer) {
        throw std::invalid_argument("AudioBuffer pointer cannot be null");
    }

    // We're supplied with an AudioSourceFactory in test.  In prod, we
    // use the default.
    if (!aAudioSourceFactory) {
        aAudioSourceFactory = CreateDefaultAudioSourceFactory();
    }

    mAudioBuffer = buffer;
    auto format = CreateFormatFromBuffer(buffer);

    auto factoryResult = aAudioSourceFactory(device, format);
    mAudioSource = std::move(factoryResult.audioSource);
    mAudioIODevice = factoryResult.ioDevice;

    if (!mAudioSource) {
        emit errorOccurred("Failed to create QAudioSource");
        return false;
    }

    if (!mAudioIODevice) {
        emit errorOccurred("Failed to start audio input");
        return false;
    }

    connect(mAudioIODevice, &QIODevice::readyRead, this, &AudioRecorder::ReadAudioData);
    emit recordingStateChanged(true);
    return true;
}

void
AudioRecorder::Stop()
{
    // TODO: Implement
}

AudioRecorder::AudioSourceFactory
AudioRecorder::CreateDefaultAudioSourceFactory()
{
    return [](const QAudioDevice& device, const QAudioFormat& format) -> AudioSourceFactoryResult {
        auto source = std::make_unique<QAudioSource>(device, format);
        // QAudioSource#start isn't virtual so we can't easily mock this call.
        // Perform it here so we can also inject the QIODevice during testing.
        auto ioDevice = source->start();
        return { .audioSource = std::move(source), .ioDevice = ioDevice };
    };
}

QAudioFormat
AudioRecorder::CreateFormatFromBuffer(const AudioBuffer* buffer)
{
    QAudioFormat format;
    format.setSampleRate(buffer->GetSampleRate());
    format.setChannelCount(buffer->GetChannelCount());
    format.setSampleFormat(QAudioFormat::Float);
    return format;
}

void
AudioRecorder::ReadAudioData()
{
    // TODO: Implement
}
