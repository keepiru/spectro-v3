#ifndef AUDIO_RECORDER_H
#define AUDIO_RECORDER_H

#include <QAudioFormat>
#include <QAudioSource>
#include <QIODevice>
#include <QObject>
#include <functional>
#include <memory>

class QAudioDevice;
class AudioBuffer;

/// @brief Captures audio from an input device and writes to an AudioBuffer.
///
/// Uses Qt Multimedia's QAudioSource to capture audio samples from
/// microphone/line-in. Runs in a separate thread managed by Qt. Supports
/// runtime device changes and dependency injection for testing.
/// Audio format (sample rate, channels) is inferred from AudioBuffer at Start().
class AudioRecorder : public QObject
{
    Q_OBJECT

  public:
    /// Factory function type for creating QAudioSource instances (for testing).
    using AudioSourceFactory =
      std::function<std::unique_ptr<QAudioSource>(const QAudioFormat&, const QAudioDevice&)>;

    /// @brief Constructs an AudioRecorder.
    /// @param parent Qt parent object for memory management.
    /// @param factory Optional factory for creating QAudioSource (for testing).
    explicit AudioRecorder(QObject* parent = nullptr,
                           AudioSourceFactory factory = AudioSourceFactory());

    ~AudioRecorder() override;

    /// @brief Starts audio capture, writing samples to the specified buffer.
    /// @param buffer The AudioBuffer to write captured samples to.
    /// @param device The audio input device to capture from.
    /// @return true if capture started successfully, false otherwise.
    /// @note Audio format (sample rate, channels) is inferred from the buffer.
    bool Start(AudioBuffer* buffer, const QAudioDevice& device);

    /// @brief Stops audio capture.
    void Stop();

  signals:
    /// @brief Emitted when recording state changes.
    /// @param isRecording true if now recording, false if stopped.
    void recordingStateChanged(bool isRecording);

    /// @brief Emitted when an error occurs during capture.
    /// @param errorMessage Description of the error.
    void errorOccurred(const QString& errorMessage);

  private:
    AudioSourceFactory mAudioSourceFactory;
    std::unique_ptr<QAudioSource> mAudioSource;
    QIODevice* mAudioIODevice = nullptr;
    AudioBuffer* mAudioBuffer = nullptr;

    /// @brief Creates the default QAudioSource factory.
    static AudioSourceFactory CreateDefaultFactory();

    /// @brief Creates a QAudioFormat from AudioBuffer settings.
    static QAudioFormat CreateFormatFromBuffer(const AudioBuffer* buffer);

    /// @brief Reads available audio data and writes to the buffer.
    void ReadAudioData();
};

#endif // AUDIO_RECORDER_H
