#include "../controllers/spectrogram_controller.h"
#include "../models/audio_buffer.h"
#include <QObject>
#include <QTest>
#include <cstddef>
#include <fft_window.h>
#include <ifft_processor.h>
#include <memory>
#include <span>
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

QTEST_MAIN(TestSpectrogramController)
#include "test_spectrogram_controller.moc"