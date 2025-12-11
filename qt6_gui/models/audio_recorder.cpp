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
    // TODO: Implement
    (void)buffer;
    (void)device;
    return false;
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
