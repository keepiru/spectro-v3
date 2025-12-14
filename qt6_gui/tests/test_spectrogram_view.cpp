#include "views/spectrogram_view.h"
#include <audio_buffer.h>
#include <spectrogram_controller.h>

#include <QTest>

class TestSpectrogramView : public QObject
{
    Q_OBJECT

  private slots:
    static void TestConstructorThrowsIfControllerIsNull()
    {
        QVERIFY_EXCEPTION_THROWN(SpectrogramView(nullptr), std::invalid_argument);
    }

    static void TestConstructor()
    {

        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer);
        const SpectrogramView view(&controller);

        QVERIFY(view.minimumWidth() > 0);
        QVERIFY(view.minimumHeight() > 0);
    }

    static void TestIsWidget()
    {
        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer);
        SpectrogramView view(&controller);
        QVERIFY(qobject_cast<QWidget*>(&view) != nullptr);
    }

    static void TestSetColorMapInvalid()
    {
        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer);
        SpectrogramView view(&controller);

        // Invalid enum value (not in defined range)
        // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
        const auto invalidValue = static_cast<SpectrogramView::ColorMapType>(999);
        QVERIFY_EXCEPTION_THROWN(view.SetColorMap(invalidValue), std::invalid_argument);
    }

    static void TestSetColorMapGrayscale()
    {
        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer);
        SpectrogramView view(&controller);

        view.SetColorMap(SpectrogramView::ColorMapType::kGrayscale);

        for (size_t i = 0; i < 256; i++) {
            const auto intensity = static_cast<uint8_t>(i);
            const auto kHave = view.GetColorMapValue(intensity);
            QCOMPARE(kHave.r, intensity);
            QCOMPARE(kHave.g, intensity);
            QCOMPARE(kHave.b, intensity);
        }
    }
};

QTEST_MAIN(TestSpectrogramView)
#include "test_spectrogram_view.moc"
