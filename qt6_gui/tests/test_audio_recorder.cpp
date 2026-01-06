#include "include/global_constants.h"
#include "models/audio_buffer.h"
#include "models/audio_recorder.h"
#include <QAudioDevice>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QSignalSpy>
#include <catch2/catch_test_macros.hpp>

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

TEST_CASE("AudioRecorder constructor succeeds", "[audio_recorder]")
{
    AudioBuffer buffer;
    const AudioRecorder recorder(buffer);
    // Constructor should not crash
}

TEST_CASE("AudioRecorder::Start throws with invalid arguments", "[audio_recorder]")
{
    AudioBuffer buffer;
    AudioRecorder recorder(buffer);

    // Invalid channel count
    REQUIRE_THROWS_AS(recorder.Start(QAudioDevice(), -1, 48000), std::invalid_argument);
    REQUIRE_THROWS_AS(recorder.Start(QAudioDevice(), 0, 48000), std::invalid_argument);
    recorder.Start(QAudioDevice(), 1, 48000);             // Does not throw
    recorder.Start(QAudioDevice(), GKMaxChannels, 48000); // Does not throw
    REQUIRE_THROWS_AS(recorder.Start(QAudioDevice(), GKMaxChannels + 1, 48000),
                      std::invalid_argument);

    // Invalid sample rate
    REQUIRE_THROWS_AS(recorder.Start(QAudioDevice(), 1, 0), std::invalid_argument);
    REQUIRE_THROWS_AS(recorder.Start(QAudioDevice(), 1, -44100), std::invalid_argument);
}

TEST_CASE("AudioRecorder::Start resets audio buffer", "[audio_recorder]")
{
    AudioBuffer buffer;
    AudioRecorder recorder(buffer);
    MockQIODevice ioDevice;

    // Add some samples to the buffer first
    buffer.AddSamples({ 0.1f, 0.2f, 0.3f, 0.4f });
    REQUIRE(buffer.GetFrameCount() == FrameCount(2));
    REQUIRE(buffer.GetSampleRate() == 44100);

    recorder.Start(QAudioDevice(), 2, 48000, &ioDevice);

    // Buffer should be reset
    REQUIRE(buffer.GetFrameCount() == FrameCount(0));
    REQUIRE(buffer.GetSampleRate() == 48000);
}

TEST_CASE("AudioRecorder::Stop when not recording is no-op", "[audio_recorder]")
{
    AudioBuffer buffer;
    AudioRecorder recorder(buffer);
    const QSignalSpy spy(&recorder, &AudioRecorder::RecordingStateChanged);
    recorder.Stop();           // Should not crash
    REQUIRE(spy.count() == 0); // No events should be emitted
}

TEST_CASE("AudioRecorder::Stop after start succeeds", "[audio_recorder]")
{
    AudioBuffer buffer;
    AudioRecorder recorder(buffer);
    MockQIODevice ioDevice;
    recorder.Start(QAudioDevice(), 1, 48000, &ioDevice);
    QSignalSpy spy(&recorder, &AudioRecorder::RecordingStateChanged);

    recorder.Stop(); // Should not crash
    REQUIRE(spy.count() == 1);
    const QList<QVariant> arguments = spy.takeFirst();
    REQUIRE(arguments.at(0).toBool() == false);

    recorder.Stop();           // Repeating should be a no-op
    REQUIRE(spy.count() == 0); // No new signals
}

TEST_CASE("AudioRecorder recording state changed signal emitted", "[audio_recorder]")
{
    AudioBuffer buffer;
    MockQIODevice ioDevice;
    AudioRecorder recorder(buffer);
    QSignalSpy spy(&recorder, &AudioRecorder::RecordingStateChanged);

    recorder.Start(QAudioDevice(), 1, 48000, &ioDevice);

    // Verify the signal is emitted
    REQUIRE(spy.count() == 1);

    // Check that the state changed to true
    const QList<QVariant> arguments = spy.takeFirst();
    REQUIRE(arguments.at(0).toBool() == true);
}

TEST_CASE("AudioRecorder audio data written to buffer", "[audio_recorder]")
{
    AudioBuffer buffer;
    MockQIODevice ioDevice;
    AudioRecorder recorder(buffer);
    recorder.Start(QAudioDevice(), 2, 48000, &ioDevice);

    // Feed in some mock audio data...
    ioDevice.SimulateAudioData({ 0.1, 0.2, 0.3, 0.4 });

    // ... then see if it comes back.
    REQUIRE(buffer.GetFrameCount() == FrameCount(2));
    REQUIRE(buffer.GetSamples(0, SampleIndex(0), SampleCount(2)) == std::vector<float>({ 0.1, 0.3 }));
    REQUIRE(buffer.GetSamples(1, SampleIndex(0), SampleCount(2)) == std::vector<float>({ 0.2, 0.4 }));

    // Add some more...
    ioDevice.SimulateAudioData({ 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 });

    // ... And it should all be there.
    REQUIRE(buffer.GetFrameCount() == FrameCount(5));
    REQUIRE(buffer.GetSamples(0, SampleIndex(0), SampleCount(5)) == std::vector<float>({ 0.1, 0.3, 0.5, 0.7, 0.9 }));
    REQUIRE(buffer.GetSamples(1, SampleIndex(0), SampleCount(5)) == std::vector<float>({ 0.2, 0.4, 0.6, 0.8, 1.0 }));
}

TEST_CASE("AudioRecorder::IsRecording", "[audio_recorder]")
{
    AudioBuffer buffer;
    MockQIODevice ioDevice;
    AudioRecorder recorder(buffer);

    REQUIRE(recorder.IsRecording() == false);

    recorder.Start(QAudioDevice(), 1, 48000, &ioDevice);
    REQUIRE(recorder.IsRecording() == true);

    recorder.Stop();
    REQUIRE(recorder.IsRecording() == false);
}
