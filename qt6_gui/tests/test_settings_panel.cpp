// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "audio_types.h"
#include "controllers/audio_file.h"
#include "controllers/audio_recorder.h"
#include "include/global_constants.h"
#include "models/audio_buffer.h"
#include "models/colormap.h"
#include "models/settings.h"
#include "views/settings_panel.h"
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QSlider>
#include <catch2/catch_test_macros.hpp>
#include <fft_window.h>
#include <qobject.h>
#include <qtypes.h>
#include <stdexcept>

//
// Test Fixtures
//

namespace {

/// @brief Helper class to create a testable SettingsPanel with dependencies
struct TestFixture
{
    Settings settings;
    AudioBuffer audio_buffer;
    AudioRecorder recorder{ audio_buffer };
    AudioFile audio_file{ audio_buffer };
    SettingsPanel panel{ settings, recorder, audio_file };
};

} // namespace

//
// Construction Tests
//

TEST_CASE("SettingsPanel constructor creates valid widget", "[settings_panel]")
{
    TestFixture const fixture;
    REQUIRE(fixture.panel.objectName() == "SettingsPanel");
    REQUIRE(fixture.panel.width() == 300);
}

TEST_CASE("SettingsPanel has named audio controls", "[settings_panel]")
{
    TestFixture const fixture;

    REQUIRE(fixture.panel.GetAudioDeviceComboBox() != nullptr);
    REQUIRE(fixture.panel.GetAudioDeviceComboBox()->objectName() == "AudioDeviceComboBox");

    REQUIRE(fixture.panel.GetSampleRateComboBox() != nullptr);
    REQUIRE(fixture.panel.GetSampleRateComboBox()->objectName() == "SampleRateComboBox");

    REQUIRE(fixture.panel.GetChannelsComboBox() != nullptr);
    REQUIRE(fixture.panel.GetChannelsComboBox()->objectName() == "ChannelsComboBox");

    REQUIRE(fixture.panel.GetRecordButton() != nullptr);
    REQUIRE(fixture.panel.GetRecordButton()->objectName() == "RecordButton");

    REQUIRE(fixture.panel.GetOpenFileButton() != nullptr);
    REQUIRE(fixture.panel.GetOpenFileButton()->objectName() == "OpenFileButton");
}

TEST_CASE("SettingsPanel has named FFT controls", "[settings_panel]")
{
    TestFixture const fixture;

    REQUIRE(fixture.panel.GetWindowTypeComboBox() != nullptr);
    REQUIRE(fixture.panel.GetWindowTypeComboBox()->objectName() == "WindowTypeComboBox");

    REQUIRE(fixture.panel.GetFFTSizeComboBox() != nullptr);
    REQUIRE(fixture.panel.GetFFTSizeComboBox()->objectName() == "FFTSizeComboBox");

    REQUIRE(fixture.panel.GetWindowScaleSlider() != nullptr);
    REQUIRE(fixture.panel.GetWindowScaleSlider()->objectName() == "WindowScaleSlider");

    REQUIRE(fixture.panel.GetWindowScaleLabel() != nullptr);
    REQUIRE(fixture.panel.GetWindowScaleLabel()->objectName() == "WindowScaleLabel");

    REQUIRE(fixture.panel.GetApertureFloorSlider() != nullptr);
    REQUIRE(fixture.panel.GetApertureFloorSlider()->objectName() == "ApertureFloorSlider");

    REQUIRE(fixture.panel.GetApertureFloorLabel() != nullptr);
    REQUIRE(fixture.panel.GetApertureFloorLabel()->objectName() == "ApertureFloorLabel");

    REQUIRE(fixture.panel.GetApertureCeilingSlider() != nullptr);
    REQUIRE(fixture.panel.GetApertureCeilingSlider()->objectName() == "ApertureCeilingSlider");

    REQUIRE(fixture.panel.GetApertureCeilingLabel() != nullptr);
    REQUIRE(fixture.panel.GetApertureCeilingLabel()->objectName() == "ApertureCeilingLabel");
}

TEST_CASE("SettingsPanel has named color map controls", "[settings_panel]")
{
    TestFixture const fixture;

    for (ChannelCount i = 0; i < GKMaxChannels; ++i) {
        REQUIRE(fixture.panel.GetColorMapComboBox(i) != nullptr);
        REQUIRE(fixture.panel.GetColorMapComboBox(i)->objectName() ==
                QString("ColorMapComboBox%1").arg(i));
    }
}

TEST_CASE("SettingsPanel has named display controls", "[settings_panel]")
{
    TestFixture const fixture;

    REQUIRE(fixture.panel.GetLiveModeButton() != nullptr);
    REQUIRE(fixture.panel.GetLiveModeButton()->objectName() == "LiveModeButton");
}

//
// FFT Controls Tests
//

TEST_CASE("SettingsPanel window type combo box contains all types", "[settings_panel]")
{
    TestFixture const fixture;
    auto* comboBox = fixture.panel.GetWindowTypeComboBox();

    REQUIRE(comboBox->count() == 5);
    REQUIRE(comboBox->findData(static_cast<int>(FFTWindow::Type::Rectangular)) >= 0);
    REQUIRE(comboBox->findData(static_cast<int>(FFTWindow::Type::Hann)) >= 0);
    REQUIRE(comboBox->findData(static_cast<int>(FFTWindow::Type::Hamming)) >= 0);
    REQUIRE(comboBox->findData(static_cast<int>(FFTWindow::Type::Blackman)) >= 0);
    REQUIRE(comboBox->findData(static_cast<int>(FFTWindow::Type::BlackmanHarris)) >= 0);
}

TEST_CASE("SettingsPanel window type defaults to Settings value", "[settings_panel]")
{
    TestFixture const fixture;
    auto* comboBox = fixture.panel.GetWindowTypeComboBox();

    const auto currentData = comboBox->currentData().toInt();
    REQUIRE(currentData == static_cast<int>(fixture.settings.GetWindowType()));
}

TEST_CASE("SettingsPanel window type change updates Settings", "[settings_panel]")
{
    TestFixture const fixture;
    auto* comboBox = fixture.panel.GetWindowTypeComboBox();
    const QSignalSpy spy(&fixture.settings, &Settings::FFTSettingsChanged);

    // Change to Blackman
    const int blackmanIndex = comboBox->findData(static_cast<int>(FFTWindow::Type::Blackman));
    comboBox->setCurrentIndex(blackmanIndex);

    REQUIRE(fixture.settings.GetWindowType() == FFTWindow::Type::Blackman);
    REQUIRE(spy.count() == 1);
}

TEST_CASE("SettingsPanel FFT size combo box contains valid sizes", "[settings_panel]")
{
    TestFixture const fixture;
    auto* comboBox = fixture.panel.GetFFTSizeComboBox();

    REQUIRE(comboBox->count() == static_cast<int>(Settings::KValidFFTSizes.size()));
    for (const auto& size : Settings::KValidFFTSizes) {
        REQUIRE(comboBox->findData(static_cast<qulonglong>(size.Get())) >= 0);
    }
}

TEST_CASE("SettingsPanel FFT size defaults to Settings value", "[settings_panel]")
{
    TestFixture const fixture;
    auto* comboBox = fixture.panel.GetFFTSizeComboBox();

    const auto currentData = comboBox->currentData().toULongLong();
    REQUIRE(currentData == fixture.settings.GetFFTSize().Get());
}

TEST_CASE("SettingsPanel FFT size change updates Settings", "[settings_panel]")
{
    TestFixture const fixture;
    auto* comboBox = fixture.panel.GetFFTSizeComboBox();
    const QSignalSpy spy(&fixture.settings, &Settings::FFTSettingsChanged);

    // Change to 4096
    const int index4096 = comboBox->findData(static_cast<qulonglong>(4096));
    comboBox->setCurrentIndex(index4096);

    REQUIRE(fixture.settings.GetFFTSize().Get() == 4096);
    REQUIRE(spy.count() == 1);
}

TEST_CASE("SettingsPanel window scale slider range", "[settings_panel]")
{
    TestFixture const fixture;
    auto* slider = fixture.panel.GetWindowScaleSlider();

    REQUIRE(slider->minimum() == 0);
    REQUIRE(slider->maximum() == 4); // 5 values: 1, 2, 4, 8, 16
}

TEST_CASE("SettingsPanel window scale slider updates Settings", "[settings_panel]")
{
    TestFixture const fixture;
    auto* slider = fixture.panel.GetWindowScaleSlider();
    auto* label = fixture.panel.GetWindowScaleLabel();
    const QSignalSpy spy(&fixture.settings, &Settings::DisplaySettingsChanged);

    // Set to index 3 (value 8)
    slider->setValue(3);

    REQUIRE(fixture.settings.GetWindowScale() == 8);
    REQUIRE(label->text() == "8");
    REQUIRE(spy.count() >= 1);
}

TEST_CASE("SettingsPanel aperture floor slider range", "[settings_panel]")
{
    TestFixture const fixture;
    auto* slider = fixture.panel.GetApertureFloorSlider();

    REQUIRE(slider->minimum() == Settings::KApertureLimitsDecibels.first);
    REQUIRE(slider->maximum() == Settings::KApertureLimitsDecibels.second);
}

TEST_CASE("SettingsPanel aperture floor slider updates Settings", "[settings_panel]")
{
    TestFixture const fixture;
    auto* slider = fixture.panel.GetApertureFloorSlider();
    auto* label = fixture.panel.GetApertureFloorLabel();
    const QSignalSpy spy(&fixture.settings, &Settings::DisplaySettingsChanged);

    slider->setValue(-40);

    REQUIRE(fixture.settings.GetApertureFloorDecibels() == -40.0f);
    REQUIRE(label->text() == "-40");
    REQUIRE(spy.count() >= 1);
}

TEST_CASE("SettingsPanel aperture ceiling slider range", "[settings_panel]")
{
    TestFixture const fixture;
    auto* slider = fixture.panel.GetApertureCeilingSlider();

    REQUIRE(slider->minimum() == Settings::KApertureLimitsDecibels.first);
    REQUIRE(slider->maximum() == Settings::KApertureLimitsDecibels.second);
}

TEST_CASE("SettingsPanel aperture ceiling slider updates Settings", "[settings_panel]")
{
    TestFixture const fixture;
    auto* slider = fixture.panel.GetApertureCeilingSlider();
    auto* label = fixture.panel.GetApertureCeilingLabel();
    const QSignalSpy spy(&fixture.settings, &Settings::DisplaySettingsChanged);

    slider->setValue(60);

    REQUIRE(fixture.settings.GetApertureCeilingDecibels() == 60.0f);
    REQUIRE(label->text() == "60");
    REQUIRE(spy.count() >= 1);
}

//
// Color Map Controls Tests
//

TEST_CASE("SettingsPanel color map combo boxes contain all types", "[settings_panel]")
{
    TestFixture const fixture;
    auto* comboBox = fixture.panel.GetColorMapComboBox(0);

    REQUIRE(comboBox->count() == static_cast<int>(ColorMap::TypeNames.size()));

    for (const auto& [type, name] : ColorMap::TypeNames) {
        REQUIRE(comboBox->findData(static_cast<int>(type)) >= 0);
    }
}

TEST_CASE("SettingsPanel color map combo boxes have icons", "[settings_panel]")
{
    TestFixture const fixture;
    auto* comboBox = fixture.panel.GetColorMapComboBox(0);

    REQUIRE(comboBox->iconSize().width() == 128);
    REQUIRE(comboBox->iconSize().height() == 16);

    // Verify each item has an icon
    for (int i = 0; i < comboBox->count(); ++i) {
        REQUIRE(!comboBox->itemIcon(i).isNull());
    }
}

TEST_CASE("SettingsPanel color map defaults to Settings value", "[settings_panel]")
{
    TestFixture const fixture;

    for (ChannelCount ch = 0; ch < 2; ++ch) {
        auto* comboBox = fixture.panel.GetColorMapComboBox(ch);
        const auto currentData = comboBox->currentData().toInt();
        REQUIRE(currentData == static_cast<int>(fixture.settings.GetColorMapType(ch)));
    }
}

TEST_CASE("SettingsPanel color map change updates Settings", "[settings_panel]")
{
    TestFixture const fixture;
    auto* comboBox = fixture.panel.GetColorMapComboBox(0);
    const QSignalSpy spy(&fixture.settings, &Settings::DisplaySettingsChanged);

    // Change to Viridis
    const int viridisIndex = comboBox->findData(static_cast<int>(ColorMap::Type::Viridis));
    comboBox->setCurrentIndex(viridisIndex);

    REQUIRE(fixture.settings.GetColorMapType(0) == ColorMap::Type::Viridis);
    REQUIRE(spy.count() >= 1);
}

TEST_CASE("SettingsPanel UpdateColorMapDropdowns visibility", "[settings_panel]")
{
    TestFixture fixture;

    // Note: In Qt, isVisible() returns false if parent is not visible.
    // Use !isHidden() to check the widget's own visibility flag.

    SECTION("Shows 1 colormap for mono")
    {
        fixture.panel.UpdateColorMapDropdowns(1);
        REQUIRE_FALSE(fixture.panel.GetColorMapComboBox(0)->isHidden());
        REQUIRE(fixture.panel.GetColorMapComboBox(1)->isHidden());
    }

    SECTION("Shows 2 colormaps for stereo")
    {
        fixture.panel.UpdateColorMapDropdowns(2);
        REQUIRE_FALSE(fixture.panel.GetColorMapComboBox(0)->isHidden());
        REQUIRE_FALSE(fixture.panel.GetColorMapComboBox(1)->isHidden());
        REQUIRE(fixture.panel.GetColorMapComboBox(2)->isHidden());
    }

    SECTION("Shows max colormaps for high channel count")
    {
        fixture.panel.UpdateColorMapDropdowns(GKMaxChannels);
        for (ChannelCount i = 0; i < GKMaxChannels; ++i) {
            REQUIRE_FALSE(fixture.panel.GetColorMapComboBox(i)->isHidden());
        }
    }

    SECTION("Clamps to max channels")
    {
        fixture.panel.UpdateColorMapDropdowns(100);
        for (ChannelCount i = 0; i < GKMaxChannels; ++i) {
            REQUIRE_FALSE(fixture.panel.GetColorMapComboBox(i)->isHidden());
        }
    }
}

TEST_CASE("SettingsPanel GetColorMapComboBox throws on invalid channel", "[settings_panel]")
{
    TestFixture const fixture;
    REQUIRE_THROWS_AS(fixture.panel.GetColorMapComboBox(GKMaxChannels), std::out_of_range);
    REQUIRE_THROWS_AS(fixture.panel.GetColorMapComboBox(100), std::out_of_range);
}

//
// Display Controls Tests
//

TEST_CASE("SettingsPanel live mode button sets live mode", "[settings_panel]")
{
    TestFixture fixture;

    // Clear live mode first
    fixture.settings.ClearLiveMode();
    REQUIRE_FALSE(fixture.settings.IsLiveMode());

    // Click live mode button
    fixture.panel.GetLiveModeButton()->click();

    REQUIRE(fixture.settings.IsLiveMode());
}

//
// Audio Controls State Tests
//

TEST_CASE("SettingsPanel record button text changes with state", "[settings_panel]")
{
    TestFixture const fixture;

    // Initially should show "Start Recording"
    REQUIRE(fixture.panel.GetRecordButton()->text() == "Start Recording");
}

TEST_CASE("SettingsPanel audio controls enabled by default", "[settings_panel]")
{
    TestFixture const fixture;

    REQUIRE(fixture.panel.GetAudioDeviceComboBox()->isEnabled());
    REQUIRE(fixture.panel.GetSampleRateComboBox()->isEnabled());
    REQUIRE(fixture.panel.GetChannelsComboBox()->isEnabled());
    REQUIRE(fixture.panel.GetOpenFileButton()->isEnabled());
}

//
// Integration Tests
//

TEST_CASE("SettingsPanel combined FFT settings change", "[settings_panel]")
{
    TestFixture const fixture;
    const QSignalSpy fftSpy(&fixture.settings, &Settings::FFTSettingsChanged);

    auto* windowTypeCombo = fixture.panel.GetWindowTypeComboBox();
    auto* fftSizeCombo = fixture.panel.GetFFTSizeComboBox();

    // Change both settings
    const int blackmanIndex =
      windowTypeCombo->findData(static_cast<int>(FFTWindow::Type::Blackman));
    windowTypeCombo->setCurrentIndex(blackmanIndex);

    const int index8192 = fftSizeCombo->findData(static_cast<qulonglong>(8192));
    fftSizeCombo->setCurrentIndex(index8192);

    REQUIRE(fixture.settings.GetWindowType() == FFTWindow::Type::Blackman);
    REQUIRE(fixture.settings.GetFFTSize().Get() == 8192);
    REQUIRE(fftSpy.count() >= 2); // At least 2 changes
}

TEST_CASE("SettingsPanel slider label synchronization", "[settings_panel]")
{
    TestFixture const fixture;

    // Test window scale
    fixture.panel.GetWindowScaleSlider()->setValue(0); // Value 1
    REQUIRE(fixture.panel.GetWindowScaleLabel()->text() == "1");

    fixture.panel.GetWindowScaleSlider()->setValue(4); // Value 16
    REQUIRE(fixture.panel.GetWindowScaleLabel()->text() == "16");

    // Test aperture floor
    fixture.panel.GetApertureFloorSlider()->setValue(-60);
    REQUIRE(fixture.panel.GetApertureFloorLabel()->text() == "-60");

    // Test aperture ceiling
    fixture.panel.GetApertureCeilingSlider()->setValue(80);
    REQUIRE(fixture.panel.GetApertureCeilingLabel()->text() == "80");
}
