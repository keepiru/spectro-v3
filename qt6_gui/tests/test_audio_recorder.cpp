#include "../models/audio_buffer.h"
#include "../models/audio_recorder.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QObject>
#include <QSignalSpy>
#include <QTest>

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
        // TODO: Implement when Start() is implemented
        QSKIP("Waiting for Start() implementation");
    }

    void TestStopWhenNotRecordingIsNoOp()
    {
        AudioRecorder recorder;
        recorder.Stop(); // Should not crash
    }

    void TestRecordingStateChangedSignalEmitted()
    {
        // TODO: Implement when Start() is implemented
        QSKIP("Waiting for Start() implementation");
    }

    void TestAudioDataWrittenToBuffer()
    {
        // TODO: Implement when ReadAudioData() is implemented
        QSKIP("Waiting for ReadAudioData() implementation");
    }
};

QTEST_MAIN(TestAudioRecorder)
#include "test_audio_recorder.moc"
