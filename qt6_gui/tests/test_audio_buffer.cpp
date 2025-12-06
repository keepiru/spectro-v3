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
    void TestConstructor()
    {
        AudioBuffer buffer(2, 44100);
        QCOMPARE(buffer.GetChannelCount(), 2);
        QCOMPARE(buffer.GetSampleRate(), 44100);
    }

    void TestConstructorThrowsOnZeroChannels()
    {
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, AudioBuffer(0, 44100));
    }

    void TestConstructorThrowsOnZeroSampleRate()
    {
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, AudioBuffer(2, 0));
    }

    void TestAddSamplesSucceedsWithValidSize()
    {
        AudioBuffer buffer(2, 44100);
        buffer.AddSamples({ 1, 2, 3, 4 });
    }

    void TestAddSamplesThrowsOnInvalidSize()
    {
        AudioBuffer buffer(2, 44100);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, buffer.AddSamples({ 1, 2, 3, 4, 5 }));
    }

    void TestGetChannelBufferSucceeds()
    {
        AudioBuffer buffer(2, 44100);
        auto& chan0 = buffer.GetChannelBuffer(0);
    }

    void TestGetChannelBufferThrowsOnInvalidChannelIndex()
    {
        AudioBuffer buffer(2, 44100);
        QVERIFY_THROWS_EXCEPTION(std::out_of_range, (void)buffer.GetChannelBuffer(size_t(2)));
    }

    void TestAddSamplesDeinterleavesToChannelBuffers()
    {
        AudioBuffer buffer(2, 44100);
        buffer.AddSamples({ 1, 2, 3, 4 });

        auto& chan0 = buffer.GetChannelBuffer(0);
        auto& chan1 = buffer.GetChannelBuffer(1);

        QCOMPARE(chan0.GetSamples(0, 2), std::vector<float>({ 1, 3 }));
        QCOMPARE(chan1.GetSamples(0, 2), std::vector<float>({ 2, 4 }));
    }

    void TestAddSamplesEmitsSignal()
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
