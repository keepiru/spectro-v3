#include "include/global_constants.h"
#include "models/audio_buffer.h"
#include "models/settings.h"
#include "views/settings_panel.h"
#include <QComboBox>
#include <QLabel>
#include <QSignalSpy>
#include <QSlider>
#include <QTest>
#include <fft_window.h>

class TestSettingsPanel : public QObject
{
    Q_OBJECT

  private slots:
    static void TestConstructor()
    {
        Settings settings;
        AudioBuffer audioBuffer;
        AudioRecorder audioRecorder(audioBuffer);
        const SettingsPanel panel(settings, audioRecorder);
        QCOMPARE(panel.width(), 300);
    }

    static void TestWindowTypeControl()
    {
        Settings settings;
        AudioBuffer audioBuffer;
        AudioRecorder audioRecorder(audioBuffer);
        SettingsPanel panel(settings, audioRecorder);

        auto* combo = panel.findChild<QComboBox*>("windowTypeCombo");
        QVERIFY(combo != nullptr);

        // Check that it has the expected items
        QCOMPARE(combo->count(), 2);
        QCOMPARE(combo->itemText(0), "Rectangular");
        QCOMPARE(combo->itemText(1), "Hann");

        // Change the value and verify settings update
        combo->setCurrentIndex(0);
        QCOMPARE(settings.GetWindowType(), FFTWindow::Type::Rectangular);

        combo->setCurrentIndex(1);
        QCOMPARE(settings.GetWindowType(), FFTWindow::Type::Hann);
    }

    static void TestFFTSizeControl()
    {
        Settings settings;
        AudioBuffer audioBuffer;
        AudioRecorder audioRecorder(audioBuffer);
        SettingsPanel panel(settings, audioRecorder);

        auto* combo = panel.findChild<QComboBox*>("fftSizeCombo");
        QVERIFY(combo != nullptr);

        // Check that it has the expected items
        QCOMPARE(combo->count(), 5);
        QCOMPARE(combo->itemText(0), "512");
        QCOMPARE(combo->itemText(1), "1024");
        QCOMPARE(combo->itemText(2), "2048");
        QCOMPARE(combo->itemText(3), "4096");
        QCOMPARE(combo->itemText(4), "8192");

        // Change the value and verify settings update
        combo->setCurrentIndex(0);
        QCOMPARE(settings.GetFFTSize(), 512);

        combo->setCurrentIndex(4);
        QCOMPARE(settings.GetFFTSize(), 8192);
    }

    static void TestWindowScaleControl()
    {
        Settings settings;
        AudioBuffer audioBuffer;
        AudioRecorder audioRecorder(audioBuffer);
        SettingsPanel panel(settings, audioRecorder);

        auto* slider = panel.findChild<QSlider*>("windowScaleSlider");
        auto* label = panel.findChild<QLabel*>("windowScaleLabel");
        QVERIFY(slider != nullptr);
        QVERIFY(label != nullptr);

        // Check range
        QCOMPARE(slider->minimum(), 0);
        QCOMPARE(slider->maximum(), 4);

        // Test each position
        slider->setValue(0);
        QCOMPARE(settings.GetWindowScale(), 1);
        QCOMPARE(label->text(), "1");

        slider->setValue(1);
        QCOMPARE(settings.GetWindowScale(), 2);
        QCOMPARE(label->text(), "2");

        slider->setValue(2);
        QCOMPARE(settings.GetWindowScale(), 4);
        QCOMPARE(label->text(), "4");

        slider->setValue(3);
        QCOMPARE(settings.GetWindowScale(), 8);
        QCOMPARE(label->text(), "8");

        slider->setValue(4);
        QCOMPARE(settings.GetWindowScale(), 16);
        QCOMPARE(label->text(), "16");
    }

    static void TestApertureMinControl()
    {
        Settings settings;
        AudioBuffer audioBuffer;
        AudioRecorder audioRecorder(audioBuffer);
        SettingsPanel panel(settings, audioRecorder);

        auto* slider = panel.findChild<QSlider*>("apertureMinSlider");
        auto* label = panel.findChild<QLabel*>("apertureMinLabel");
        QVERIFY(slider != nullptr);
        QVERIFY(label != nullptr);

        // Check range
        QCOMPARE(slider->minimum(), -80);
        QCOMPARE(slider->maximum(), 30);

        // Test setting values
        slider->setValue(-50);
        QCOMPARE(settings.GetApertureMinDecibels(), -50.0f);
        QCOMPARE(label->text(), "-50 dB");

        slider->setValue(10);
        QCOMPARE(settings.GetApertureMinDecibels(), 10.0f);
        QCOMPARE(label->text(), "10 dB");
    }

    static void TestApertureMaxControl()
    {
        Settings settings;
        AudioBuffer audioBuffer;
        AudioRecorder audioRecorder(audioBuffer);
        SettingsPanel panel(settings, audioRecorder);

        auto* slider = panel.findChild<QSlider*>("apertureMaxSlider");
        auto* label = panel.findChild<QLabel*>("apertureMaxLabel");
        QVERIFY(slider != nullptr);
        QVERIFY(label != nullptr);

        // Check range
        QCOMPARE(slider->minimum(), -80);
        QCOMPARE(slider->maximum(), 30);

        // Test setting values
        slider->setValue(-20);
        QCOMPARE(settings.GetApertureMaxDecibels(), -20.0f);
        QCOMPARE(label->text(), "-20 dB");

        slider->setValue(20);
        QCOMPARE(settings.GetApertureMaxDecibels(), 20.0f);
        QCOMPARE(label->text(), "20 dB");
    }

    static void TestColorMapControls()
    {
        Settings settings;
        AudioBuffer audioBuffer;
        AudioRecorder audioRecorder(audioBuffer);
        SettingsPanel panel(settings, audioRecorder);

        // Test all 6 color map combos
        for (size_t i = 0; i < 6; i++) {
            const QString objectName = QString("colorMapCombo%1").arg(i);
            auto* combo = panel.findChild<QComboBox*>(objectName);
            QVERIFY2(combo != nullptr, qPrintable(QString("Color map combo %1 not found").arg(i)));

            // Check that it has the 7 implemented color map types
            QCOMPARE(combo->count(), 7); // White, Red, Green, Blue, Cyan, Magenta, Yellow

            // Check icon size
            QCOMPARE(combo->iconSize(), QSize(128, 16));

            // Verify each combo has icons
            for (int j = 0; j < combo->count(); j++) {
                QVERIFY2(
                  !combo->itemIcon(j).isNull(),
                  qPrintable(QString("Icon missing for color map %1, item %2").arg(i).arg(j)));
            }

            // Test changing color map (only for valid channels)
            if (i < gkMaxChannels) {
                combo->setCurrentIndex(static_cast<int>(Settings::ColorMapType::Red));
                QCOMPARE(settings.GetColorMap(i), Settings::ColorMapType::Red);

                combo->setCurrentIndex(static_cast<int>(Settings::ColorMapType::Blue));
                QCOMPARE(settings.GetColorMap(i), Settings::ColorMapType::Blue);
            }
        }
    }

    static void TestInitialValues()
    {
        Settings settings;
        // Set some initial values
        settings.SetFFTSettings(4096, FFTWindow::Type::Rectangular);
        settings.SetWindowScale(8);
        settings.SetApertureMinDecibels(-60.0f);
        settings.SetApertureMaxDecibels(10.0f);
        settings.SetColorMap(0, Settings::ColorMapType::Magenta);

        // Create panel and verify controls reflect the settings
        AudioBuffer audioBuffer;
        AudioRecorder audioRecorder(audioBuffer);
        SettingsPanel panel(settings, audioRecorder);

        auto* windowTypeCombo = panel.findChild<QComboBox*>("windowTypeCombo");
        QCOMPARE(windowTypeCombo->currentData().toInt(),
                 static_cast<int>(FFTWindow::Type::Rectangular));

        auto* fftSizeCombo = panel.findChild<QComboBox*>("fftSizeCombo");
        QCOMPARE(fftSizeCombo->currentData().toULongLong(), 4096);

        auto* windowScaleSlider = panel.findChild<QSlider*>("windowScaleSlider");
        QCOMPARE(windowScaleSlider->value(), 3); // 8 is at index 3

        auto* apertureMinSlider = panel.findChild<QSlider*>("apertureMinSlider");
        QCOMPARE(apertureMinSlider->value(), -60);

        auto* apertureMaxSlider = panel.findChild<QSlider*>("apertureMaxSlider");
        QCOMPARE(apertureMaxSlider->value(), 10);

        auto* colorMapCombo0 = panel.findChild<QComboBox*>("colorMapCombo0");
        QCOMPARE(colorMapCombo0->currentData().toInt(),
                 static_cast<int>(Settings::ColorMapType::Magenta));
    }

    static void TestSignalConnections()
    {
        Settings settings;
        AudioBuffer audioBuffer;
        AudioRecorder audioRecorder(audioBuffer);
        SettingsPanel panel(settings, audioRecorder);

        // Test that signals are emitted when controls change
        QSignalSpy fftSpy(&settings, &Settings::FFTSettingsChanged);
        QSignalSpy windowScaleSpy(&settings, &Settings::WindowScaleChanged);
        QSignalSpy apertureSpy(&settings, &Settings::ApertureSettingsChanged);

        auto* fftSizeCombo = panel.findChild<QComboBox*>("fftSizeCombo");
        fftSizeCombo->setCurrentIndex(0);
        QCOMPARE(fftSpy.count(), 1);

        auto* windowTypeCombo = panel.findChild<QComboBox*>("windowTypeCombo");
        windowTypeCombo->setCurrentIndex(0);
        QCOMPARE(fftSpy.count(), 2);

        auto* windowScaleSlider = panel.findChild<QSlider*>("windowScaleSlider");
        windowScaleSlider->setValue(4);
        QCOMPARE(windowScaleSpy.count(), 1);

        auto* apertureMinSlider = panel.findChild<QSlider*>("apertureMinSlider");
        apertureMinSlider->setValue(-40);
        QCOMPARE(apertureSpy.count(), 1);

        auto* apertureMaxSlider = panel.findChild<QSlider*>("apertureMaxSlider");
        apertureMaxSlider->setValue(15);
        QCOMPARE(apertureSpy.count(), 2);
    }
};

QTEST_MAIN(TestSettingsPanel)
#include "test_settings_panel.moc"
