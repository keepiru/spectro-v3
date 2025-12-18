#include "include/global_constants.h"
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
        settings.SetFFTSettings(2048, FFTWindow::Type::Hann);
        const QSignalSpy spy(&settings, &Settings::FFTSettingsChanged);

        settings.SetFFTSettings(4096, FFTWindow::Type::Rectangular);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(settings.GetFFTSize(), 4096);
        QCOMPARE(settings.GetWindowType(), FFTWindow::Type::Rectangular);
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

    static void GetApertureMinDecibels()
    {
        const Settings settings;
        QCOMPARE(settings.GetApertureMinDecibels(), -30.0f);
    }

    static void GetApertureMaxDecibels()
    {
        const Settings settings;
        QCOMPARE(settings.GetApertureMaxDecibels(), 30.0f);
    }

    static void TestSetColorMapInvalid()
    {
        Settings settings;

        // Invalid enum value (not in defined range)
        // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
        const auto invalidValue = static_cast<Settings::ColorMapType>(999);
        QVERIFY_EXCEPTION_THROWN(settings.SetColorMap(0, invalidValue), std::invalid_argument);
    }

    // The linter complains about cognitive complexity because it's looking at
    // the macro expansion of QCOMPARE.  That's silly.  This is fine.
    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    static void TestDefaultColorMaps()
    {
        const Settings settings;

        for (size_t i = 0; i < 256; i++) {
            const auto intensity = static_cast<uint8_t>(i);
            // Channel 0: Cyan
            QCOMPARE(settings.GetColorMapValue(0, i).r, 0);
            QCOMPARE(settings.GetColorMapValue(0, i).g, intensity);
            QCOMPARE(settings.GetColorMapValue(0, i).b, intensity);
            // Channel 1: Red
            QCOMPARE(settings.GetColorMapValue(1, i).r, intensity);
            QCOMPARE(settings.GetColorMapValue(1, i).g, 0);
            QCOMPARE(settings.GetColorMapValue(1, i).b, 0);
            // Rest of channels: White
            for (size_t ch = 2; ch < gkMaxChannels; ch++) {
                QCOMPARE(settings.GetColorMapValue(ch, i).r, intensity);
                QCOMPARE(settings.GetColorMapValue(ch, i).g, intensity);
                QCOMPARE(settings.GetColorMapValue(ch, i).b, intensity);
            }
        }
    }

    static void TestGetColorMapValueOutOfRange()
    {
        const Settings settings;

        // Index out of range should throw
        QVERIFY_EXCEPTION_THROWN((void)settings.GetColorMapValue(gkMaxChannels, 0),
                                 std::out_of_range);
    }
};

QTEST_MAIN(TestSettings)
#include "test_settings.moc"
