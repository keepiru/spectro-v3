#include "../models/audio_buffer.h"
#include <QSignalSpy>
#include <QTest>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <vector>

class TestAudioBuffer : public QObject
{
    Q_OBJECT

  private slots:
    void testConstructor();
    void testConstructorThrowsOnZeroChannels();
    void testGetChannelCount();
    void testGetSampleRate();
    void testAddSamplesMonoEmitsSignal();
    void testAddSamplesStereoDeinterleaves();
    void testAddSamplesThrowsOnInvalidSize();
    void testGetNumSamples();
    void testGetNumSamplesThrowsOnInvalidChannel();
    void testGetSamples();
    void testGetSamplesThrowsOnInvalidChannel();
    void testGetChannelBuffer();
    void testGetChannelBufferThrowsOnInvalidChannel();
};

void
TestAudioBuffer::testConstructor()
{
    AudioBuffer buffer(2, 44100);
    QCOMPARE(buffer.GetChannelCount(), 2UL);
    QCOMPARE(buffer.GetSampleRate(), 44100UL);
}

void
TestAudioBuffer::testConstructorThrowsOnZeroChannels()
{
    QVERIFY_EXCEPTION_THROWN(AudioBuffer(0, 44100), std::invalid_argument);
}

void
TestAudioBuffer::testGetChannelCount()
{
    AudioBuffer buffer(1, 48000);
    QCOMPARE(buffer.GetChannelCount(), 1UL);
}

void
TestAudioBuffer::testGetSampleRate()
{
    AudioBuffer buffer(2, 96000);
    QCOMPARE(buffer.GetSampleRate(), 96000UL);
}

void
TestAudioBuffer::testAddSamplesMonoEmitsSignal()
{
    AudioBuffer buffer(1, 44100);
    QSignalSpy spy(&buffer, &AudioBuffer::dataAvailable);

    std::vector<float> samples = { 0.1f, 0.2f, 0.3f, 0.4f };
    buffer.AddSamples(samples);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<size_t>(), 4UL);
}

void
TestAudioBuffer::testAddSamplesStereoDeinterleaves()
{
    AudioBuffer buffer(2, 44100);

    // Interleaved stereo: L0, R0, L1, R1, L2, R2
    std::vector<float> samples = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
    buffer.AddSamples(samples);

    // Check left channel (0)
    auto leftSamples = buffer.GetSamples(0, 0, 3);
    QCOMPARE(leftSamples, std::vector<float>({ 1.0f, 3.0f, 5.0f }));

    // Check right channel (1)
    auto rightSamples = buffer.GetSamples(1, 0, 3);
    QCOMPARE(rightSamples, std::vector<float>({ 2.0f, 4.0f, 6.0f }));
}

void
TestAudioBuffer::testAddSamplesThrowsOnInvalidSize()
{
    AudioBuffer buffer(2, 44100);

    // 5 samples not divisible by 2 channels
    std::vector<float> samples = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    QVERIFY_EXCEPTION_THROWN(buffer.AddSamples(samples), std::invalid_argument);
}

void
TestAudioBuffer::testGetNumSamples()
{
    AudioBuffer buffer(2, 44100);
    std::vector<float> samples = { 1.0f, 2.0f, 3.0f, 4.0f };
    buffer.AddSamples(samples);

    QCOMPARE(buffer.GetNumSamples(0), 2UL);
    QCOMPARE(buffer.GetNumSamples(1), 2UL);
}

void
TestAudioBuffer::testGetNumSamplesThrowsOnInvalidChannel()
{
    AudioBuffer buffer(2, 44100);
    QVERIFY_EXCEPTION_THROWN(buffer.GetNumSamples(2), std::out_of_range);
    QVERIFY_EXCEPTION_THROWN(buffer.GetNumSamples(100), std::out_of_range);
}

void
TestAudioBuffer::testGetSamples()
{
    AudioBuffer buffer(1, 44100);
    std::vector<float> samples = { 0.1f, 0.2f, 0.3f, 0.4f };
    buffer.AddSamples(samples);

    auto retrieved = buffer.GetSamples(0, 1, 2);
    QCOMPARE(retrieved, std::vector<float>({ 0.2f, 0.3f }));
}

void
TestAudioBuffer::testGetSamplesThrowsOnInvalidChannel()
{
    AudioBuffer buffer(2, 44100);
    QVERIFY_EXCEPTION_THROWN(buffer.GetSamples(2, 0, 10), std::out_of_range);
}

void
TestAudioBuffer::testGetChannelBuffer()
{
    AudioBuffer buffer(2, 44100);
    std::vector<float> samples = { 1.0f, 2.0f, 3.0f, 4.0f };
    buffer.AddSamples(samples);

    SampleBuffer& channelBuffer = buffer.GetChannelBuffer(0);
    QCOMPARE(channelBuffer.NumSamples(), 2UL);
    QCOMPARE(channelBuffer.GetSampleRate(), 44100UL);
}

void
TestAudioBuffer::testGetChannelBufferThrowsOnInvalidChannel()
{
    AudioBuffer buffer(2, 44100);
    QVERIFY_EXCEPTION_THROWN(buffer.GetChannelBuffer(2), std::out_of_range);
}

QTEST_MAIN(TestAudioBuffer)
#include "test_audio_buffer.moc"
