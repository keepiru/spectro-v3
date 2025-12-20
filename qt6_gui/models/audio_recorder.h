#pragma once

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
/// microphone/line-in. Supports runtime device changes and dependency injection
/// for testing. Audio format (sample rate, channels) is inferred from
/// AudioBuffer at Start().
class AudioRecorder : public QObject
{
    Q_OBJECT

  public:
    struct AudioSourceFactoryResult
    {
        std::unique_ptr<QAudioSource> audio_source;
        QIODevice* io_device = nullptr;
    };

    /// Factory function type for creating QAudioSource instances (for testing).
    /// We can't easily mock QAudioSource#start because it's not virtual, so we
    /// also start the QAudioSource here and return the QIODevice.
    using AudioSourceFactory =
      std::function<AudioSourceFactoryResult(const QAudioDevice&, const QAudioFormat&)>;

    /// @brief Constructs an AudioRecorder.
    /// @param aParent Qt parent object for memory management.
    explicit AudioRecorder(QObject* aParent = nullptr);

    ~AudioRecorder() override;

    /// @brief Starts audio capture, writing samples to the specified buffer.
    /// @param aAudioBuffer The AudioBuffer to write captured samples to.
    /// @param aQAudioDevice The audio input device to capture from.
    /// @param aMockQIODevice Mock audio IO device for testing.  (optional)
    /// @return true if capture started successfully, false otherwise.
    /// @note Audio format (sample rate, channels) is inferred from aAudioBuffer.
    bool Start(AudioBuffer* aAudioBuffer,
               const QAudioDevice& aQAudioDevice,
               QIODevice* aMockQIODevice = nullptr);

    /// @brief Stops audio capture.
    /// @note no-op unless a capture is in progress.
    void Stop();

    /// @brief Returns whether audio capture is currently active.
    /// @return true if recording, false otherwise.
    [[nodiscard]] bool IsRecording() const;

  signals:
    /// @brief Emitted when recording state changes.
    /// @param aIsRecording true if now recording, false if stopped.
    void RecordingStateChanged(bool aIsRecording);

    /// @brief Emitted when an error occurs during capture.
    /// @param aErrorMessage Description of the error.
    void ErrorOccurred(const QString& aErrorMessage);

  private:
    std::unique_ptr<QAudioSource> mAudioSource;
    QIODevice* mAudioIODevice = nullptr;
    AudioBuffer* mAudioBuffer = nullptr;

    /// @brief Creates a QAudioFormat from AudioBuffer settings.
    static QAudioFormat CreateFormatFromBuffer(const AudioBuffer* aBuffer);

    /// @brief Reads available audio data and writes to the aBuffer.
    void ReadAudioData();
};
