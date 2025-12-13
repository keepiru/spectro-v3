#include "audio_recorder.h"
#include "audio_buffer.h"
#include <QAudioDevice>

AudioRecorder::AudioRecorder(QObject* aParent)
  : QObject(aParent)
{
}

AudioRecorder::~AudioRecorder()
{
    Stop();
}

bool
AudioRecorder::Start(AudioBuffer* aAudioBuffer,
                     const QAudioDevice& aQAudioDevice,
                     QIODevice* aMockQIODevice)
{
    if (!aAudioBuffer) {
        throw std::invalid_argument("AudioBuffer pointer cannot be null");
    }

    mAudioBuffer = aAudioBuffer;
    auto format = CreateFormatFromBuffer(aAudioBuffer);

    mAudioSource = std::make_unique<QAudioSource>(aQAudioDevice, format);

    if (!mAudioSource) {
        emit errorOccurred("Failed to create QAudioSource");
        return false;
    }

    // In test, this value will be discarded and replaced with the mock.
    // Unfortunately we can't override start() to inject the mock.
    mAudioIODevice = mAudioSource->start();

    if (!mAudioIODevice) {
        emit errorOccurred("Failed to start audio input");
        return false;
    }

    // In test, discard the real QIODevice and replace it with the mock.
    if (aMockQIODevice) {
        mAudioIODevice = aMockQIODevice;
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

QAudioFormat
AudioRecorder::CreateFormatFromBuffer(const AudioBuffer* aBuffer)
{
    QAudioFormat format;
    format.setSampleRate(aBuffer->GetSampleRate());
    format.setChannelCount(aBuffer->GetChannelCount());
    format.setSampleFormat(QAudioFormat::Float);
    return format;
}

void
AudioRecorder::ReadAudioData()
{
    // TODO: Implement
}
