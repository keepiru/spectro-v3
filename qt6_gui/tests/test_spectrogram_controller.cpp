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
};

QTEST_GUILESS_MAIN(TestSpectrogramController)
#include "test_spectrogram_controller.moc"