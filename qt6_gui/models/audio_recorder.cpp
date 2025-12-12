#include "audio_recorder.h"

#include <QAudioDevice>

#include "audio_buffer.h"

AudioRecorder::AudioRecorder(QObject* parent, AudioSourceFactory factory)
  : QObject(parent)
  , mAudioSourceFactory(factory ? std::move(factory) : CreateDefaultFactory())
{
}

AudioRecorder::~AudioRecorder()
{
    Stop();
}

bool
AudioRecorder::Start(AudioBuffer* buffer, const QAudioDevice& device)
{
    if (!buffer) {
        throw std::invalid_argument("AudioBuffer pointer cannot be null");
    }

    mAudioBuffer = buffer;
    auto format = CreateFormatFromBuffer(buffer);

    mAudioSource = mAudioSourceFactory(format, device);
    if (!mAudioSource) {
        emit errorOccurred("Failed to create QAudioSource");
        return false;
    }

    mAudioIODevice = mAudioSource->start();
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
AudioRecorder::CreateDefaultFactory()
{
    // TODO: Implement
    return nullptr;
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
