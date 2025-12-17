#include "models/settings.h"
#include <QObject>
#include <QSignalSpy>
#include <QTest>
#include <fft_window.h>

class TestSettings : public QObject
{
    Q_OBJECT

  private slots:
    static void TestSetFFTSettingsEmitsSignal()
    {
        Settings settings;
        settings.SetFFTSettings(2048, FFTWindow::Type::kHann);
        const QSignalSpy spy(&settings, &Settings::FFTSettingsChanged);

        settings.SetFFTSettings(4096, FFTWindow::Type::kRectangular);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(settings.GetFFTSize(), 4096);
        QCOMPARE(settings.GetWindowType(), FFTWindow::Type::kRectangular);
    }

    static void TestSetFFTSettingsNoSignalIfSameValues()
    {
        Settings settings;
        const QSignalSpy spy(&settings, &Settings::FFTSettingsChanged);
        auto size = settings.GetFFTSize();
        auto type = settings.GetWindowType();

        settings.SetFFTSettings(size, type); // Set to default again

        QCOMPARE(spy.count(), 0);
    }

    static void TestSetFFTSettingsThrowsOnZeroWindowSize()
    {
        Settings settings;
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
                                 settings.SetFFTSettings(0, settings.GetWindowType()));
    }

    static void TestSetFFTSettingsThrowsOnNonPowerOfTwoSize()
    {
        Settings settings;
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
                                 settings.SetFFTSettings(255, settings.GetWindowType()));
    }

    static void TestSetWindowStrideEmitsSignal()
    {
        Settings settings;
        const QSignalSpy spy(&settings, &Settings::WindowStrideChanged);

        settings.SetWindowStride(512);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(settings.GetWindowStride(), 512);
    }

    static void TestSetWindowStrideNoSignalIfSameValue()
    {
        Settings settings;
        const QSignalSpy spy(&settings, &Settings::WindowStrideChanged);

        settings.SetWindowStride(settings.GetWindowStride()); // Set to same value again

        QCOMPARE(spy.count(), 0);
    }

    static void TestSetWindowStrideThrowsOnZero()
    {
        Settings settings;
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, settings.SetWindowStride(0));
    }
};

QTEST_MAIN(TestSettings)
#include "test_settings.moc"
