// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "controllers/audio_file.h"
#include "controllers/audio_recorder.h"
#include "include/global_constants.h"
#include "models/audio_buffer.h"
#include "models/settings.h"
#include "views/settings_panel.h"
#include <QComboBox>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QSignalSpy>
#include <QSlider>
#include <QSpinBox>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <fft_window.h>

TEST_CASE("SettingsPanel constructor", "[settings_panel]")
{
    Settings settings;
    AudioBuffer audioBuffer;
    AudioRecorder audioRecorder(audioBuffer);
    AudioFile audioFile(audioBuffer);
    const SettingsPanel panel(settings, audioRecorder, audioFile);
    REQUIRE(panel.width() == 300);
}

TEST_CASE("SettingsPanel window type control", "[settings_panel]")
{
    Settings settings;
    AudioBuffer audioBuffer;
    AudioRecorder audioRecorder(audioBuffer);
    AudioFile audioFile(audioBuffer);
    SettingsPanel const panel(settings, audioRecorder, audioFile);

    auto* combo = panel.findChild<QComboBox*>("windowTypeCombo");
    REQUIRE(combo != nullptr);

    // Check that it has the expected items
    REQUIRE(combo->count() == 5);
    REQUIRE(combo->itemText(0) == "Rectangular");
    REQUIRE(combo->itemText(1) == "Hann");
    REQUIRE(combo->itemText(2) == "Hamming");
    REQUIRE(combo->itemText(3) == "Blackman");
    REQUIRE(combo->itemText(4) == "Blackman-Harris");

    // Change the value and verify settings update
    combo->setCurrentIndex(0);
    REQUIRE(settings.GetWindowType() == FFTWindow::Type::Rectangular);

    combo->setCurrentIndex(1);
    REQUIRE(settings.GetWindowType() == FFTWindow::Type::Hann);

    combo->setCurrentIndex(2);
    REQUIRE(settings.GetWindowType() == FFTWindow::Type::Hamming);

    combo->setCurrentIndex(3);
    REQUIRE(settings.GetWindowType() == FFTWindow::Type::Blackman);

    combo->setCurrentIndex(4);
    REQUIRE(settings.GetWindowType() == FFTWindow::Type::BlackmanHarris);
}

TEST_CASE("SettingsPanel FFT size control", "[settings_panel]")
{
    Settings settings;
    AudioBuffer audioBuffer;
    AudioRecorder audioRecorder(audioBuffer);
    AudioFile audioFile(audioBuffer);
    SettingsPanel const panel(settings, audioRecorder, audioFile);

    auto* combo = panel.findChild<QComboBox*>("fftSizeCombo");
    REQUIRE(combo != nullptr);

    // Check that it has the expected items
    REQUIRE(combo->count() == 6);
    REQUIRE(combo->itemText(0) == "512");
    REQUIRE(combo->itemText(1) == "1024");
    REQUIRE(combo->itemText(2) == "2048");
    REQUIRE(combo->itemText(3) == "4096");
    REQUIRE(combo->itemText(4) == "8192");
    REQUIRE(combo->itemText(5) == "16384");

    // Change the value and verify settings update
    combo->setCurrentIndex(0);
    REQUIRE(settings.GetFFTSize() == 512);

    combo->setCurrentIndex(4);
    REQUIRE(settings.GetFFTSize() == 8192);
}

TEST_CASE("SettingsPanel window scale control", "[settings_panel]")
{
    Settings settings;
    AudioBuffer audioBuffer;
    AudioRecorder audioRecorder(audioBuffer);
    AudioFile audioFile(audioBuffer);
    SettingsPanel const panel(settings, audioRecorder, audioFile);

    auto* slider = panel.findChild<QSlider*>("windowScaleSlider");
    auto* label = panel.findChild<QLabel*>("windowScaleLabel");
    REQUIRE(slider != nullptr);
    REQUIRE(label != nullptr);

    // Check range
    REQUIRE(slider->minimum() == 0);
    REQUIRE(slider->maximum() == 4);

    // Test each position
    slider->setValue(0);
    REQUIRE(settings.GetWindowScale() == 1);
    REQUIRE(label->text() == "1");

    slider->setValue(1);
    REQUIRE(settings.GetWindowScale() == 2);
    REQUIRE(label->text() == "2");

    slider->setValue(2);
    REQUIRE(settings.GetWindowScale() == 4);
    REQUIRE(label->text() == "4");

    slider->setValue(3);
    REQUIRE(settings.GetWindowScale() == 8);
    REQUIRE(label->text() == "8");

    slider->setValue(4);
    REQUIRE(settings.GetWindowScale() == 16);
    REQUIRE(label->text() == "16");
}

TEST_CASE("SettingsPanel aperture min control", "[settings_panel]")
{
    Settings settings;
    AudioBuffer audioBuffer;
    AudioRecorder audioRecorder(audioBuffer);
    AudioFile audioFile(audioBuffer);
    SettingsPanel const panel(settings, audioRecorder, audioFile);

    auto* slider = panel.findChild<QSlider*>("apertureMinSlider");
    auto* label = panel.findChild<QLabel*>("apertureMinLabel");
    REQUIRE(slider != nullptr);
    REQUIRE(label != nullptr);

    // Check range
    REQUIRE(slider->minimum() == -80);
    REQUIRE(slider->maximum() == 100);

    // Test setting values
    slider->setValue(-50);
    REQUIRE(settings.GetApertureMinDecibels() == -50.0f);
    REQUIRE(label->text() == "-50 dB");

    slider->setValue(10);
    REQUIRE(settings.GetApertureMinDecibels() == 10.0f);
    REQUIRE(label->text() == "10 dB");
}

TEST_CASE("SettingsPanel aperture max control", "[settings_panel]")
{
    Settings settings;
    AudioBuffer audioBuffer;
    AudioRecorder audioRecorder(audioBuffer);
    AudioFile audioFile(audioBuffer);
    SettingsPanel const panel(settings, audioRecorder, audioFile);

    auto* slider = panel.findChild<QSlider*>("apertureMaxSlider");
    auto* label = panel.findChild<QLabel*>("apertureMaxLabel");
    REQUIRE(slider != nullptr);
    REQUIRE(label != nullptr);

    // Check range
    REQUIRE(slider->minimum() == -80);
    REQUIRE(slider->maximum() == 100);

    // Test setting values
    slider->setValue(-20);
    REQUIRE(settings.GetApertureMaxDecibels() == -20.0f);
    REQUIRE(label->text() == "-20 dB");

    slider->setValue(20);
    REQUIRE(settings.GetApertureMaxDecibels() == 20.0f);
    REQUIRE(label->text() == "20 dB");
}

TEST_CASE("SettingsPanel color map controls", "[settings_panel]")
{
    Settings settings;
    AudioBuffer audioBuffer;
    AudioRecorder audioRecorder(audioBuffer);
    AudioFile audioFile(audioBuffer);
    SettingsPanel const panel(settings, audioRecorder, audioFile);

    // Test all 6 color map combos
    for (size_t i = 0; i < 6; i++) {
        const QString objectName = QString("colorMapCombo%1").arg(i);
        auto* combo = panel.findChild<QComboBox*>(objectName);
        REQUIRE(combo != nullptr);

        // Check that it exposes all color map types (see Settings::ColorMapType)
        REQUIRE(combo->count() == 19);

        // Check icon size
        REQUIRE(combo->iconSize() == QSize(128, 16));

        // Verify each combo has icons
        for (int j = 0; j < combo->count(); j++) {
            REQUIRE(!combo->itemIcon(j).isNull());
        }

        // Test changing color map (only for valid channels)
        if (i < GKMaxChannels) {
            combo->setCurrentIndex(static_cast<int>(Settings::ColorMapType::Red));
            REQUIRE(settings.GetColorMap(i) == Settings::ColorMapType::Red);

            combo->setCurrentIndex(static_cast<int>(Settings::ColorMapType::Blue));
            REQUIRE(settings.GetColorMap(i) == Settings::ColorMapType::Blue);
        }
    }
}

TEST_CASE("SettingsPanel initial values", "[settings_panel]")
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
    AudioFile audioFile(audioBuffer);
    SettingsPanel const panel(settings, audioRecorder, audioFile);

    auto* windowTypeCombo = panel.findChild<QComboBox*>("windowTypeCombo");
    REQUIRE(windowTypeCombo->currentData().toInt() ==
            static_cast<int>(FFTWindow::Type::Rectangular));

    auto* fftSizeCombo = panel.findChild<QComboBox*>("fftSizeCombo");
    REQUIRE(fftSizeCombo->currentData().toInt() == 4096);

    auto* windowScaleSlider = panel.findChild<QSlider*>("windowScaleSlider");
    REQUIRE(windowScaleSlider->value() == 3); // 8 is at index 3

    auto* apertureMinSlider = panel.findChild<QSlider*>("apertureMinSlider");
    REQUIRE(apertureMinSlider->value() == -60);

    auto* apertureMaxSlider = panel.findChild<QSlider*>("apertureMaxSlider");
    REQUIRE(apertureMaxSlider->value() == 10);

    auto* colorMapCombo0 = panel.findChild<QComboBox*>("colorMapCombo0");
    REQUIRE(colorMapCombo0->currentData().toInt() ==
            static_cast<int>(Settings::ColorMapType::Magenta));
}

TEST_CASE("SettingsPanel signal connections", "[settings_panel]")
{
    Settings settings;
    AudioBuffer audioBuffer;
    AudioRecorder audioRecorder(audioBuffer);
    AudioFile audioFile(audioBuffer);
    SettingsPanel const panel(settings, audioRecorder, audioFile);

    // Test that signals are emitted when controls change
    QSignalSpy const fftSpy(&settings, &Settings::FFTSettingsChanged);
    QSignalSpy const displaySpy(&settings, &Settings::DisplaySettingsChanged);

    auto* fftSizeCombo = panel.findChild<QComboBox*>("fftSizeCombo");
    fftSizeCombo->setCurrentIndex(0);
    REQUIRE(fftSpy.count() == 1);
    REQUIRE(displaySpy.count() == 1);

    auto* windowTypeCombo = panel.findChild<QComboBox*>("windowTypeCombo");
    windowTypeCombo->setCurrentIndex(0);
    REQUIRE(fftSpy.count() == 2);
    REQUIRE(displaySpy.count() == 2);

    auto* windowScaleSlider = panel.findChild<QSlider*>("windowScaleSlider");
    windowScaleSlider->setValue(4);
    REQUIRE(displaySpy.count() == 3);

    auto* apertureMinSlider = panel.findChild<QSlider*>("apertureMinSlider");
    apertureMinSlider->setValue(-40);
    REQUIRE(displaySpy.count() == 4);

    auto* apertureMaxSlider = panel.findChild<QSlider*>("apertureMaxSlider");
    apertureMaxSlider->setValue(15);
    REQUIRE(displaySpy.count() == 5);
}

TEST_CASE("SettingsPanel audio device control", "[settings_panel]")
{
    Settings settings;
    AudioBuffer audioBuffer;
    AudioRecorder audioRecorder(audioBuffer);
    AudioFile audioFile(audioBuffer);
    SettingsPanel const panel(settings, audioRecorder, audioFile);

    auto* combo = panel.findChild<QComboBox*>("audioDeviceCombo");
    REQUIRE(combo != nullptr);

    // Should have at least one device (system should have some audio input)
    // On CI systems there may be no devices, so just verify the control exists
    REQUIRE(combo->count() >= 0);
}

TEST_CASE("SettingsPanel sample rate control", "[settings_panel]")
{
    Settings settings;
    AudioBuffer audioBuffer;
    AudioRecorder audioRecorder(audioBuffer);
    AudioFile audioFile(audioBuffer);
    SettingsPanel const panel(settings, audioRecorder, audioFile);

    auto* combo = panel.findChild<QComboBox*>("sampleRateCombo");
    REQUIRE(combo != nullptr);

    // Sample rates should be populated based on device capabilities
    // On CI systems there may be no devices, so just verify the control exists
    REQUIRE(combo->count() >= 0);

    // If there are sample rates, verify they have Hz suffix in display text
    if (combo->count() > 0) {
        REQUIRE(combo->itemText(0).contains("Hz"));
    }
}

TEST_CASE("SettingsPanel channel count control", "[settings_panel]")
{
    Settings settings;
    AudioBuffer audioBuffer;
    AudioRecorder audioRecorder(audioBuffer);
    AudioFile audioFile(audioBuffer);
    SettingsPanel const panel(settings, audioRecorder, audioFile);

    auto* spinBox = panel.findChild<QSpinBox*>("channelCountSpinBox");
    REQUIRE(spinBox != nullptr);

    // Check that max is clamped to application's GKMaxChannels
    REQUIRE(spinBox->maximum() <= static_cast<int>(GKMaxChannels));

    // Check that minimum is at least 1
    REQUIRE(spinBox->minimum() >= 1);

    // Current value should be within range
    REQUIRE(spinBox->value() >= spinBox->minimum());
    REQUIRE(spinBox->value() <= spinBox->maximum());
}

TEST_CASE("SettingsPanel recording button", "[settings_panel]")
{
    Settings settings;
    AudioBuffer audioBuffer;
    AudioRecorder audioRecorder(audioBuffer);
    AudioFile audioFile(audioBuffer);
    SettingsPanel const panel(settings, audioRecorder, audioFile);

    auto* button = panel.findChild<QPushButton*>("recordingButton");
    REQUIRE(button != nullptr);

    // Initially should show "Start Recording" (not recording)
    REQUIRE(button->text() == QString("Start Recording"));
}

TEST_CASE("SettingsPanel controls disabled while recording", "[settings_panel]")
{
    Settings settings;
    AudioBuffer audioBuffer;
    AudioRecorder audioRecorder(audioBuffer);
    AudioFile audioFile(audioBuffer);
    SettingsPanel const panel(settings, audioRecorder, audioFile);

    auto* deviceCombo = panel.findChild<QComboBox*>("audioDeviceCombo");
    auto* sampleRateCombo = panel.findChild<QComboBox*>("sampleRateCombo");
    auto* channelSpinBox = panel.findChild<QSpinBox*>("channelCountSpinBox");
    auto* recordingButton = panel.findChild<QPushButton*>("recordingButton");

    REQUIRE(deviceCombo != nullptr);
    REQUIRE(sampleRateCombo != nullptr);
    REQUIRE(channelSpinBox != nullptr);
    REQUIRE(recordingButton != nullptr);

    // Initially not recording - controls should be enabled
    REQUIRE(deviceCombo->isEnabled());
    REQUIRE(sampleRateCombo->isEnabled());
    REQUIRE(channelSpinBox->isEnabled());

    // Simulate recording state change by emitting signal
    emit audioRecorder.RecordingStateChanged(true); // NOLINT(misc-include-cleaner)

    // Controls should now be disabled
    REQUIRE(!deviceCombo->isEnabled());
    REQUIRE(!sampleRateCombo->isEnabled());
    REQUIRE(!channelSpinBox->isEnabled());
    REQUIRE(recordingButton->text() == QString("Stop Recording"));

    // Simulate recording stopped
    emit audioRecorder.RecordingStateChanged(false);

    // Controls should be enabled again
    REQUIRE(deviceCombo->isEnabled());
    REQUIRE(sampleRateCombo->isEnabled());
    REQUIRE(channelSpinBox->isEnabled());
    REQUIRE(recordingButton->text() == QString("Start Recording"));
}

TEST_CASE("SettingsPanel open file button", "[settings_panel]")
{
    Settings settings;
    AudioBuffer audioBuffer;
    AudioRecorder audioRecorder(audioBuffer);
    AudioFile audioFile(audioBuffer);
    SettingsPanel const panel(settings, audioRecorder, audioFile);

    auto* button = panel.findChild<QPushButton*>("openFileButton");
    REQUIRE(button != nullptr);
    REQUIRE(button->text() == QString("Open File"));
}

TEST_CASE("SettingsPanel live mode button", "[settings_panel]")
{
    Settings settings;
    AudioBuffer audioBuffer;
    AudioRecorder audioRecorder(audioBuffer);
    AudioFile audioFile(audioBuffer);
    SettingsPanel const panel(settings, audioRecorder, audioFile);

    auto* button = panel.findChild<QPushButton*>("liveModeButton");
    REQUIRE(button != nullptr);
    REQUIRE(button->text() == QString("Live Mode"));
}

TEST_CASE("SettingsPanel live mode button functionality", "[settings_panel]")
{
    Settings settings;
    AudioBuffer audioBuffer;
    AudioRecorder audioRecorder(audioBuffer);
    AudioFile audioFile(audioBuffer);
    const SettingsPanel panel(settings, audioRecorder, audioFile);

    auto* button = panel.findChild<QPushButton*>("liveModeButton");
    REQUIRE(button != nullptr);

    // Set to false first
    settings.SetLiveMode(false);
    REQUIRE(settings.IsLiveMode() == false);

    // Click button
    button->click();

    // Should now be true
    REQUIRE(settings.IsLiveMode() == true);
}
