#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "views/spectrogram_view.h"

#include <QTest>

class TestSpectrogramView : public QObject
{
    Q_OBJECT

  private slots:
    static void TestConstructor()
    {

        const Settings settings;
        const AudioBuffer audioBuffer;
        SpectrogramController controller(settings, audioBuffer);
        const SpectrogramView view(controller);

        QVERIFY(view.minimumWidth() > 0);
        QVERIFY(view.minimumHeight() > 0);
    }

    static void TestIsWidget()
    {
        const Settings settings;
        const AudioBuffer audioBuffer;
        SpectrogramController controller(settings, audioBuffer);
        SpectrogramView view(controller);
        QVERIFY(qobject_cast<QWidget*>(&view) != nullptr);
    }
};

QTEST_MAIN(TestSpectrogramView)
#include "test_spectrogram_view.moc"
