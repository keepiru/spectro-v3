#include "controllers/spectrogram_controller.h"
#include "include/global_constants.h"
#include "models/audio_buffer.h"
#include "views/spectrogram_view.h"

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
        QVERIFY_EXCEPTION_THROWN(view.SetColorMap(0, invalidValue), std::invalid_argument);
    }

    // The linter complains about cognitive complexity because it's looking at
    // the macro expansion of QCOMPARE.  That's silly.  This is fine.
    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    static void TestDefaultColorMaps()
    {
        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer);
        const SpectrogramView view(&controller);

        for (size_t i = 0; i < 256; i++) {
            const auto intensity = static_cast<uint8_t>(i);
            // Channel 0: Cyan
            QCOMPARE(view.GetColorMapValue(0, i).r, 0);
            QCOMPARE(view.GetColorMapValue(0, i).g, intensity);
            QCOMPARE(view.GetColorMapValue(0, i).b, intensity);
            // Channel 1: Red
            QCOMPARE(view.GetColorMapValue(1, i).r, intensity);
            QCOMPARE(view.GetColorMapValue(1, i).g, 0);
            QCOMPARE(view.GetColorMapValue(1, i).b, 0);
            // Rest of channels: White
            for (size_t ch = 2; ch < gkMaxChannels; ch++) {
                QCOMPARE(view.GetColorMapValue(ch, i).r, intensity);
                QCOMPARE(view.GetColorMapValue(ch, i).g, intensity);
                QCOMPARE(view.GetColorMapValue(ch, i).b, intensity);
            }
        }
    }

    static void TestGetColorMapValueOutOfRange()
    {
        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer);
        const SpectrogramView view(&controller);

        // Index out of range should throw
        QVERIFY_EXCEPTION_THROWN((void)view.GetColorMapValue(gkMaxChannels, 0), std::out_of_range);
    }
};

QTEST_MAIN(TestSpectrogramView)
#include "test_spectrogram_view.moc"
