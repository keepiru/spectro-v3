#include <QObject>
#include <QTest>
#include <audio_buffer.h>
#include <cstddef>
#include <fft_window.h>
#include <ifft_processor.h>
#include <memory>
#include <span>
#include <spectrogram_controller.h>
#include <stdexcept>
#include <vector>

class TestSpectrogramController : public QObject
{
    Q_OBJECT

  private slots:
    static void TestConstructor()
    {
        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer, nullptr, nullptr);
    }

    static void TestSetWindowStride()
    {
        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer, nullptr, nullptr);

        QCOMPARE(controller.GetWindowStride(), 0);

        controller.SetWindowStride(512);
        QCOMPARE(controller.GetWindowStride(), 512);
    }

    static void TestSetWindowStrideThorwsOnZeroStride()
    {
        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer, nullptr, nullptr);

        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, controller.SetWindowStride(0));
    }
};

QTEST_GUILESS_MAIN(TestSpectrogramController)
#include "test_spectrogram_controller.moc"