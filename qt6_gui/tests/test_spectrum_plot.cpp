#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "views/spectrum_plot.h"

#include <QTest>

class TestSpectrumPlot : public QObject
{
    Q_OBJECT

  private slots:
    static void TestConstructor()
    {

        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer);
        const SpectrumPlot plot(&controller);
        QVERIFY(plot.minimumWidth() > 0);
        QVERIFY(plot.minimumHeight() > 0);
    }

    static void TestConstructorThrowsOnNullController()
    {
        // NOLINTNEXTLINE(misc-const-correctness) - Object is being constructed in exception test
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, SpectrumPlot plot(nullptr));
    }

    static void TestIsWidget()
    {
        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer);
        SpectrumPlot plot(&controller);
        QVERIFY(qobject_cast<QWidget*>(&plot) != nullptr);
    }
};

QTEST_MAIN(TestSpectrumPlot)
#include "test_spectrum_plot.moc"
