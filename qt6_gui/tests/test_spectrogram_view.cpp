#include "views/spectrogram_view.h"
#include <audio_buffer.h>
#include <spectrogram_controller.h>

#include <QTest>

class TestSpectrogramView : public QObject
{
    Q_OBJECT

  private slots:
    void TestConstructorThrowsIfControllerIsNull()
    {
        QVERIFY_EXCEPTION_THROWN(SpectrogramView(nullptr), std::invalid_argument);
    }

    void testConstructor()
    {

        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer);
        SpectrogramView view(&controller);

        QVERIFY(view.minimumWidth() > 0);
        QVERIFY(view.minimumHeight() > 0);
    }

    void testIsWidget()
    {
        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer);
        SpectrogramView view(&controller);
        QVERIFY(qobject_cast<QWidget*>(&view) != nullptr);
    }
};

QTEST_MAIN(TestSpectrogramView)
#include "test_spectrogram_view.moc"
