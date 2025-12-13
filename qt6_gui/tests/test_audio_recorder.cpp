#include "../models/audio_buffer.h"
#include "../models/audio_recorder.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QObject>
#include <QSignalSpy>
#include <QTest>

/**
 * @brief Mock QIODevice to simulate audio input for testing.
 */
class MockQIODevice : public QIODevice
{
  public:
    explicit MockQIODevice() { open(QIODevice::ReadOnly); }

    /**
     * @brief Simulates incoming audio data by appending to the internal buffer.
     * @param samples Vector of float samples to append.
     */
    void SimulateAudioData(const std::vector<float>& samples)
    {
        // Type punning is intentional: converting float samples to byte stream
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        const char* dataPtr = reinterpret_cast<const char*>(samples.data());
        const auto dataSize =
          static_cast<qint64>(samples.size()) * static_cast<qint64>(sizeof(float));
        mBuffer.append(dataPtr, dataSize);
        emit readyRead();
    }

    /**
     * @brief Reads data from the internal buffer.
     * @param data Pointer to the destination buffer.
     * @param maxlen Maximum number of bytes to read.
     * @return Number of bytes actually read.
     */
    qint64 readData(char* data, qint64 maxlen) override
    {
        const qint64 bytesToRead = qMin(maxlen, static_cast<qint64>(mBuffer.size()));
        std::memcpy(data, mBuffer.constData(), static_cast<size_t>(bytesToRead));
        mBuffer.remove(0, static_cast<int>(bytesToRead));
        return bytesToRead;
    }

    // Required for the QIODevice interface, but not used in this mock.
    qint64 writeData(const char* /*data*/, qint64 /*len*/) override { return -1; }

  private:
    QByteArray mBuffer;
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
        const QSignalSpy spy(&recorder, &AudioRecorder::recordingStateChanged);
        recorder.Stop();          // Should not crash
        QCOMPARE(spy.count(), 0); // No events should be emitted
    }

    static void TestStopAfterStartSucceeds()
    {
        AudioBuffer buffer(1, 48000);
        AudioRecorder recorder;
        MockQIODevice ioDevice;
        recorder.Start(&buffer, QAudioDevice(), &ioDevice);
        QSignalSpy spy(&recorder, &AudioRecorder::recordingStateChanged);

        recorder.Stop(); // Should not crash
        QCOMPARE(spy.count(), 1);
        const QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.at(0).toBool(), false);

        recorder.Stop();          // Repeating should be a no-op
        QCOMPARE(spy.count(), 0); // No new signals
    }

    static void TestRecordingStateChangedSignalEmitted()
    {
        AudioBuffer buffer(1, 48000);
        MockQIODevice ioDevice;
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
        AudioBuffer buffer(2, 48000);
        MockQIODevice ioDevice;
        AudioRecorder recorder;
        recorder.Start(&buffer, QAudioDevice(), &ioDevice);

        // Feed in some mock audio data...
        ioDevice.SimulateAudioData({ 0.1, 0.2, 0.3, 0.4 });

        // ... then see if it comes back.
        QCOMPARE(buffer.NumSamples(), 2);
        QCOMPARE(buffer.GetSamples(0, 0, 2), std::vector<float>({ 0.1, 0.3 }));
        QCOMPARE(buffer.GetSamples(1, 0, 2), std::vector<float>({ 0.2, 0.4 }));

        // Add some more...
        ioDevice.SimulateAudioData({ 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 });

        // ... And it should all be there.
        QCOMPARE(buffer.NumSamples(), 5);
        QCOMPARE(buffer.GetSamples(0, 0, 5), std::vector<float>({ 0.1, 0.3, 0.5, 0.7, 0.9 }));
        QCOMPARE(buffer.GetSamples(1, 0, 5), std::vector<float>({ 0.2, 0.4, 0.6, 0.8, 1.0 }));
    }
};

QTEST_MAIN(TestAudioRecorder)
#include "test_audio_recorder.moc"
