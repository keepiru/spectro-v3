#include "../models/audio_buffer.h"
#include "../models/audio_recorder.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QObject>
#include <QSignalSpy>
#include <QTest>

class MockQIODevice : public QIODevice
{
  public:
    explicit MockQIODevice(QObject* parent) {}

    /**
     * @brief Create an AudioSourceFactory based on this MockQIODevice.
     * @return A factory that creates a QAudioSource instance along with this MockQIODevice.
     */
    AudioRecorder::AudioSourceFactory CreateAudioSourceFactory()
    {
        return [this](const QAudioDevice& device,
                      const QAudioFormat& format) -> AudioRecorder::AudioSourceFactoryResult {
            auto source = std::make_unique<QAudioSource>(format);
            return { .audioSource = std::move(source), .ioDevice = this };
        };
    }

    // Stub the QIODevice interface
    qint64 readData(char* /*data*/, qint64 /*maxlen*/) override { return -1; }
    qint64 writeData(const char* /*data*/, qint64 /*len*/) { return -1; }
};

class TestAudioRecorder : public QObject
{
    Q_OBJECT

  private slots:
    void TestConstructorSucceeds()
    {
        AudioRecorder recorder;
        // Constructor should not crash
    }

    void TestStartWithNullBufferThrows()
    {
        AudioRecorder recorder;
        QAudioDevice device = QMediaDevices::defaultAudioInput();
        QVERIFY_EXCEPTION_THROWN(recorder.Start(nullptr, device), std::invalid_argument);
    }

    void TestStartWithValidBufferSucceeds()
    {
        // This tests without a MockQIODevice, so it will use the default
        // device.  This ensures the default AudioSourceFactory works, but it
        // might break if we run tests on a system with no devices.  We'll cross
        // that bridge when we get there.
        AudioBuffer buffer(1, 48000);
        AudioRecorder recorder;
        recorder.Start(&buffer, QAudioDevice());
    }

    void TestStopWhenNotRecordingIsNoOp()
    {
        AudioRecorder recorder;
        recorder.Stop(); // Should not crash
    }

    void TestRecordingStateChangedSignalEmitted()
    {
        AudioBuffer buffer(1, 48000);
        MockQIODevice ioDevice(nullptr);
        AudioRecorder recorder;
        QSignalSpy spy(&recorder, &AudioRecorder::recordingStateChanged);

        recorder.Start(&buffer, QAudioDevice(), ioDevice.CreateAudioSourceFactory());

        // Verify signal emitted
        QCOMPARE(spy.count(), 1);

        // Check that the state changed to true
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.at(0).toBool(), true);
    }

    void TestAudioDataWrittenToBuffer()
    {
        // TODO: Implement when ReadAudioData() is implemented
        QSKIP("Waiting for ReadAudioData() implementation");
    }
};

QTEST_MAIN(TestAudioRecorder)
#include "test_audio_recorder.moc"
