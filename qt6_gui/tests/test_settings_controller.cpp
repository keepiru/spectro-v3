#include "controllers/settings_controller.h"
#include <QAudioDevice>
#include <QSignalSpy>
#include <QTest>

class TestSettingsController : public QObject
{
    Q_OBJECT

  private slots:
    static void TestStartRecording()
    {
        Settings settings;
        SettingsController controller(&settings);

        const QSignalSpy spy(&controller, &SettingsController::StartAudioRecording);

        const QAudioDevice testDevice; // Default device for testing
        const int testSampleRate = 48000;
        const int testChannelCount = 1;

        QCOMPARE(spy.count(), 0);

        controller.StartRecording(testDevice, testSampleRate, testChannelCount);

        QCOMPARE(settings.GetInputDevice(), testDevice);
        QCOMPARE(settings.GetSampleRate(), testSampleRate);
        QCOMPARE(settings.GetChannelCount(), testChannelCount);
        QCOMPARE(spy.count(), 1);
    }

    static void TestStartRecordingInvalidParameters()
    {
        Settings settings;
        SettingsController controller(&settings);

        QVERIFY_EXCEPTION_THROWN(controller.StartRecording(QAudioDevice(), 0, 2),
                                 std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(controller.StartRecording(QAudioDevice(), 44100, 0),
                                 std::invalid_argument);
    }
};

QTEST_MAIN(TestSettingsController)
#include "test_settings_controller.moc"