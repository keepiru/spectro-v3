#include "include/global_constants.h"
#include "models/audio_buffer.h"
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
        AudioBuffer const buffer;
        QCOMPARE(buffer.GetChannelCount(), 2);
        QCOMPARE(buffer.GetSampleRate(), 44100);
    }

    // NOLINTNEXTLINE(readability-function-cognitive-complexity) -- QVERIFY macro expansion
    static void TestResetThrowsOnInvalidArguments()
    {
        AudioBuffer buffer;

        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, buffer.Reset(-1, 44100));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, buffer.Reset(0, 44100));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, buffer.Reset(gkMaxChannels + 1, 44100));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, buffer.Reset(2, 0));
    }

    static void TestAddSamplesSucceedsWithValidSize()
    {
        AudioBuffer buffer;
        buffer.AddSamples({ 1, 2, 3, 4 });
    }

    static void TestAddSamplesThrowsOnInvalidSize()
    {
        AudioBuffer buffer;
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, buffer.AddSamples({ 1, 2, 3, 4, 5 }));
    }

    static void TestGetSamplesThrowsIfInsufficientSamples()
    {
        AudioBuffer buffer;
        buffer.AddSamples({ 1, 2, 3, 4 });
        QVERIFY_THROWS_EXCEPTION(std::out_of_range, (void)buffer.GetSamples(1, 1, 4));
    }

    static void TestGetSamplesThrowsOnInvalidChannelIndex()
    {
        AudioBuffer const buffer;
        (void)buffer.GetSamples(1, 0, 0); // No exception
        QVERIFY_THROWS_EXCEPTION(std::out_of_range, (void)buffer.GetSamples(2, 0, 0));
    }

    static void TestAddSamplesDeinterleavesToChannelBuffers()
    {
        AudioBuffer buffer;
        buffer.AddSamples({ 1, 2, 3, 4 });

        QCOMPARE(buffer.GetSamples(0, 0, 2), std::vector<float>({ 1, 3 }));
        QCOMPARE(buffer.GetSamples(1, 0, 2), std::vector<float>({ 2, 4 }));
    }

    static void TestAddSamplesEmitsSignal()
    {
        AudioBuffer buffer;
        QSignalSpy spy(&buffer, &AudioBuffer::DataAvailable);
        buffer.AddSamples({ 1, 2, 3, 4 });

        QCOMPARE(spy.count(), 1);
        auto firstCallArgs = spy.takeFirst();
        auto have = firstCallArgs.takeFirst().value<size_t>();
        QCOMPARE(have, 2);
    }

    static void TestResetClearsSamples()
    {
        AudioBuffer buffer;
        buffer.AddSamples({ 1, 2, 3, 4 });

        QCOMPARE(buffer.NumSamples(), 2);

        buffer.Reset(2, 44100);
        QCOMPARE(buffer.NumSamples(), 0);
        QVERIFY_THROWS_EXCEPTION(std::out_of_range, (void)buffer.GetSamples(0, 0, 1));

        // Also test after changing channel count
        buffer.AddSamples({ 5, 6, 7, 8 });
        buffer.Reset(1, 44100);

        QCOMPARE(buffer.NumSamples(), 0);
    }

    static void TestResetChangesChannelCountAndSampleRate()
    {
        AudioBuffer buffer;

        QCOMPARE(buffer.GetChannelCount(), 2);
        QCOMPARE(buffer.GetSampleRate(), 44100);

        buffer.Reset(1, 22050);

        QCOMPARE(buffer.GetChannelCount(), 1);
        QCOMPARE(buffer.GetSampleRate(), 22050);
    }

    static void TestResetEmitsSignal()
    {
        AudioBuffer buffer;
        const QSignalSpy spy(&buffer, &AudioBuffer::DataAvailable);

        QCOMPARE(spy.count(), 0);

        buffer.Reset(2, 44100);
        QCOMPARE(spy.count(), 1);

        buffer.AddSamples({ 1, 2, 3, 4 });
        QCOMPARE(spy.count(), 2);

        buffer.Reset(2, 44100);
        QCOMPARE(spy.count(), 3);
    }
};

QTEST_GUILESS_MAIN(TestAudioBuffer)
#include "test_audio_buffer.moc"
