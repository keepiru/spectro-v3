#include "audio_recorder.h"
#include "audio_buffer.h"
#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSource>
#include <QByteArray>
#include <QIODevice>
#include <QObject>
#include <QtGlobal>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <vector>

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
    if (!mAudioBuffer || !mAudioIODevice) {
        // These should be set during Start(), and this callback shouldn't
        // happen unless we're started and recording.
        throw std::runtime_error("AudioRecorder::ReadAudioData called when not recording");
    }

    const auto bytesAvailable = mAudioIODevice->bytesAvailable();
    if (bytesAvailable % sizeof(float) != 0) {
        throw std::runtime_error(
          "AudioRecorder::ReadAudioData: Available bytes not divisible by sample size");
    }

    // Read it into a QByteArray, then convert to float vector.
    const QByteArray audioData = mAudioIODevice->read(bytesAvailable);
    const auto sampleCount = static_cast<size_t>(audioData.size() / sizeof(float));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto* sampleData = reinterpret_cast<const float*>(audioData.constData());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const std::vector<float> samples(sampleData, sampleData + sampleCount);

    // Then send it to the AudioBuffer.
    mAudioBuffer->AddSamples(samples);
}
