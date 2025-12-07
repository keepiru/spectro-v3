#include "../models/audio_buffer.h"
#include <QObject>
#include <QSignalSpy>
#include <QTest>
#include <cstddef>
#include <stdexcept>
#include <vector>

class TestAudioBuffer : public QObject
{
    Q_OBJECT

  private slots:
    static void TestConstructor()
    {
        AudioBuffer const buffer(2, 44100);
        QCOMPARE(buffer.GetChannelCount(), 2);
        QCOMPARE(buffer.GetSampleRate(), 44100);
    }

    static void TestConstructorThrowsOnZeroChannels()
    {
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, AudioBuffer(0, 44100));
    }

    static void TestConstructorThrowsOnZeroSampleRate()
    {
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, AudioBuffer(2, 0));
    }

    static void TestAddSamplesSucceedsWithValidSize()
    {
        AudioBuffer buffer(2, 44100);
        buffer.AddSamples({ 1, 2, 3, 4 });
    }

    static void TestAddSamplesThrowsOnInvalidSize()
    {
        AudioBuffer buffer(2, 44100);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, buffer.AddSamples({ 1, 2, 3, 4, 5 }));
    }

    static void TestGetSamplesFillsZeros()
    {
        AudioBuffer buffer(2, 44100);
        buffer.AddSamples({ 1, 2, 3, 4 });
        QCOMPARE(buffer.GetSamples(0, -2, 4), std::vector<float>({ 0, 0, 1, 3 }));
        QCOMPARE(buffer.GetSamples(1, 1, 4), std::vector<float>({ 4, 0, 0, 0 }));
    }

    static void TestGetSamplesThrowsOnInvalidChannelIndex()
    {
        AudioBuffer const buffer(2, 44100);
        (void)buffer.GetSamples(1, 0, 0); // No exception
        QVERIFY_THROWS_EXCEPTION(std::out_of_range, (void)buffer.GetSamples(2, 0, 0));
    }

    static void TestAddSamplesDeinterleavesToChannelBuffers()
    {
        AudioBuffer buffer(2, 44100);
        buffer.AddSamples({ 1, 2, 3, 4 });

        QCOMPARE(buffer.GetSamples(0, 0, 2), std::vector<float>({ 1, 3 }));
        QCOMPARE(buffer.GetSamples(1, 0, 2), std::vector<float>({ 2, 4 }));
    }

    static void TestAddSamplesEmitsSignal()
    {
        AudioBuffer buffer(2, 44100);
        QSignalSpy spy(&buffer, &AudioBuffer::dataAvailable);
        buffer.AddSamples({ 1, 2, 3, 4 });

        QCOMPARE(spy.count(), 1);
        auto firstCallArgs = spy.takeFirst();
        auto have = firstCallArgs.takeFirst().value<size_t>();
        QCOMPARE(have, 2);
    }
};

QTEST_MAIN(TestAudioBuffer)
#include "test_audio_buffer.moc"
