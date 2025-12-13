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
    explicit MockQIODevice(QObject* /*parent*/) {}

    // Stub the QIODevice interface
    qint64 readData(char* /*data*/, qint64 /*maxlen*/) override { return -1; }
    qint64 writeData(const char* /*data*/, qint64 /*len*/) override { return -1; }
};

class TestAudioRecorder : public QObject
{
    Q_OBJECT

  private slots:
    static void TestConstructorSucceeds()
    {
        const AudioRecorder recorder;
        // Constructor should not crash
    }

    static void TestStartWithNullBufferThrows()
    {
        AudioRecorder recorder;
        const QAudioDevice device = QMediaDevices::defaultAudioInput();
        QVERIFY_EXCEPTION_THROWN(recorder.Start(nullptr, device), std::invalid_argument);
    }

    static void TestStartWithValidBufferSucceeds()
    {
        // This tests without a MockQIODevice, so it will use the default
        // device.  This ensures the default AudioSourceFactory works, but it
        // might break if we run tests on a system with no devices.  We'll cross
        // that bridge when we get there.
        AudioBuffer buffer(1, 48000);
        AudioRecorder recorder;
        recorder.Start(&buffer, QAudioDevice());
    }

    static void TestStopWhenNotRecordingIsNoOp()
    {
        AudioRecorder recorder;
        recorder.Stop(); // Should not crash
    }

    static void TestRecordingStateChangedSignalEmitted()
    {
        AudioBuffer buffer(1, 48000);
        MockQIODevice ioDevice(nullptr);
        AudioRecorder recorder;
        QSignalSpy spy(&recorder, &AudioRecorder::recordingStateChanged);

        recorder.Start(&buffer, QAudioDevice(), &ioDevice);

        // Verify the signal is emitted
        QCOMPARE(spy.count(), 1);

        // Check that the state changed to true
        const QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.at(0).toBool(), true);
    }

    static void TestAudioDataWrittenToBuffer()
    {
        // TODO: Implement when ReadAudioData() is implemented
        QSKIP("Waiting for ReadAudioData() implementation");
    }
};

QTEST_MAIN(TestAudioRecorder)
#include "test_audio_recorder.moc"
