// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "views/settings_panel.h"
#include "audio_types.h"
#include "controllers/audio_file.h"
#include "controllers/audio_file_reader.h"
#include "controllers/settings_controller.h"
#include "include/global_constants.h"
#include "models/colormap.h"
#include "models/settings.h"
#include <QAudioDevice>
#include <QComboBox>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMediaDevices>
#include <QObject>
#include <QProgressDialog>
#include <QSize>
#include <QSlider>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <Qt>
#include <algorithm>
#include <array>
#include <cstddef>
#include <fft_window.h>
#include <stdexcept>
#include <utility>

namespace {
constexpr int KPanelWidth = 300;
constexpr int KPanelMargin = 8;
constexpr int KPanelSpacing = 8;
constexpr int KLabelMinWidth = 30;
constexpr int KProgressMaxPercent = 100;
constexpr int KColorMapIconWidth = 128;
constexpr int KColorMapIconHeight = 16;

/// @brief Map WindowScale index to value
constexpr std::array<WindowScale, 5> KWindowScaleValues = { 1, 2, 4, 8, 16 };

/// @brief Find the index of a window scale value
int
FindWindowScaleIndex(WindowScale aScale)
{
    for (size_t i = 0; i < KWindowScaleValues.size(); ++i) {
        if (KWindowScaleValues.at(i) == aScale) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

} // namespace

SettingsPanel::SettingsPanel(Settings& aSettings,
                             SettingsController& aSettingsController,
                             AudioFile& aAudioFile,
                             QWidget* aParent)
  : QWidget(aParent)
  , mSettings(aSettings)
  , mSettingsController(aSettingsController)
  , mAudioFile(aAudioFile)
  , mAudioControlsGroup(CreateAudioControlsGroup())
{
    setFixedWidth(KPanelWidth);
    setObjectName("SettingsPanel");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(KPanelMargin, KPanelMargin, KPanelMargin, KPanelMargin);
    mainLayout->setSpacing(KPanelSpacing);

    // Add group boxes

    mainLayout->addWidget(mAudioControlsGroup);
    mainLayout->addWidget(CreateFFTControlsGroup());
    CreateColorMapControlsGroup();
    mainLayout->addWidget(mColorMapControlsGroup);
    mainLayout->addWidget(CreateDisplayControlsGroup());

    // Add stretch to push everything to top
    mainLayout->addStretch();

    // Initialize state
    OnRecordingStateChanged(mSettingsController.IsRecording());
}

QGroupBox*
SettingsPanel::CreateAudioControlsGroup()
{
    auto* group = new QGroupBox("Audio Source", this);
    group->setObjectName("AudioControlsGroup");
    auto* layout = new QVBoxLayout(group);

    // Open file button
    mOpenFileButton = new QPushButton("Open File", group);
    mOpenFileButton->setObjectName("OpenFileButton");
    layout->addWidget(mOpenFileButton);
    connect(mOpenFileButton, &QPushButton::clicked, this, &SettingsPanel::OpenFile);

    // Form layout for device settings
    auto* formLayout = new QFormLayout();
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // Audio device combo box
    mAudioDeviceComboBox = new QComboBox(group);
    mAudioDeviceComboBox->setObjectName("AudioDeviceComboBox");
    formLayout->addRow("Audio Device:", mAudioDeviceComboBox);
    PopulateAudioDevices();

    // Sample rate combo box
    mSampleRateComboBox = new QComboBox(group);
    mSampleRateComboBox->setObjectName("SampleRateComboBox");
    formLayout->addRow("Sample Rate:", mSampleRateComboBox);

    // Channels combo box
    mChannelsComboBox = new QComboBox(group);
    mChannelsComboBox->setObjectName("ChannelsComboBox");
    formLayout->addRow("Channels:", mChannelsComboBox);

    // Connect device selection to update sample rates and channels
    connect(mAudioDeviceComboBox,
            &QComboBox::currentIndexChanged,
            this,
            &SettingsPanel::PopulateSampleRates);
    connect(mAudioDeviceComboBox,
            &QComboBox::currentIndexChanged,
            this,
            &SettingsPanel::PopulateChannels);

    // Initial population
    PopulateSampleRates();
    PopulateChannels();

    layout->addLayout(formLayout);

    // Record button
    mRecordButton = new QPushButton("Start Recording", group);
    mRecordButton->setObjectName("RecordButton");
    layout->addWidget(mRecordButton);
    connect(mRecordButton, &QPushButton::clicked, this, &SettingsPanel::ToggleRecording);

    return group;
}

QGroupBox*
SettingsPanel::CreateFFTControlsGroup()
{
    auto* group = new QGroupBox("FFT", this);
    group->setObjectName("FFTControlsGroup");
    auto* formLayout = new QFormLayout(group);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // Window type combo box
    mWindowTypeComboBox = new QComboBox(group);
    mWindowTypeComboBox->setObjectName("WindowTypeComboBox");
    mWindowTypeComboBox->addItem("Rectangular", static_cast<int>(FFTWindow::Type::Rectangular));
    mWindowTypeComboBox->addItem("Hann", static_cast<int>(FFTWindow::Type::Hann));
    mWindowTypeComboBox->addItem("Hamming", static_cast<int>(FFTWindow::Type::Hamming));
    mWindowTypeComboBox->addItem("Blackman", static_cast<int>(FFTWindow::Type::Blackman));
    mWindowTypeComboBox->addItem("Blackman-Harris",
                                 static_cast<int>(FFTWindow::Type::BlackmanHarris));

    // Set default selection
    const int windowTypeIndex =
      mWindowTypeComboBox->findData(static_cast<int>(mSettings.GetWindowType()));
    if (windowTypeIndex >= 0) {
        mWindowTypeComboBox->setCurrentIndex(windowTypeIndex);
    }
    formLayout->addRow("Window Type:", mWindowTypeComboBox);

    // FFT size combo box
    mFFTSizeComboBox = new QComboBox(group);
    mFFTSizeComboBox->setObjectName("FFTSizeComboBox");
    for (const auto& size : Settings::KValidFFTSizes) {
        mFFTSizeComboBox->addItem(QString::number(size.Get()), static_cast<qulonglong>(size.Get()));
    }
    const int fftSizeIndex =
      mFFTSizeComboBox->findData(static_cast<qulonglong>(mSettings.GetFFTSize().Get()));
    if (fftSizeIndex >= 0) {
        mFFTSizeComboBox->setCurrentIndex(fftSizeIndex);
    }
    formLayout->addRow("FFT Size:", mFFTSizeComboBox);

    // Connect window type and FFT size changes
    auto updateFFTSettings = [this]() {
        const auto windowType =
          static_cast<FFTWindow::Type>(mWindowTypeComboBox->currentData().toInt());
        const auto fftSize = static_cast<FFTSize>(mFFTSizeComboBox->currentData().toInt());
        mSettings.SetFFTSettings(fftSize, windowType);
    };
    connect(mWindowTypeComboBox, &QComboBox::currentIndexChanged, this, updateFFTSettings);
    connect(mFFTSizeComboBox, &QComboBox::currentIndexChanged, this, updateFFTSettings);

    // Window scale slider
    auto* scaleLayout = new QHBoxLayout();
    mWindowScaleSlider = new QSlider(Qt::Horizontal, group);
    mWindowScaleSlider->setObjectName("WindowScaleSlider");
    mWindowScaleSlider->setRange(0, static_cast<int>(KWindowScaleValues.size()) - 1);
    mWindowScaleSlider->setValue(FindWindowScaleIndex(mSettings.GetWindowScale()));
    mWindowScaleLabel = new QLabel(QString::number(mSettings.GetWindowScale()), group);
    mWindowScaleLabel->setObjectName("WindowScaleLabel");
    mWindowScaleLabel->setMinimumWidth(KLabelMinWidth);
    scaleLayout->addWidget(mWindowScaleSlider);
    scaleLayout->addWidget(mWindowScaleLabel);
    formLayout->addRow("Window Scale:", scaleLayout);

    connect(mWindowScaleSlider, &QSlider::valueChanged, this, [this](int aValue) {
        const WindowScale scale = KWindowScaleValues.at(static_cast<size_t>(aValue));
        mWindowScaleLabel->setText(QString::number(scale));
        mSettings.SetWindowScale(scale);
    });

    // Aperture floor slider
    auto* floorLayout = new QHBoxLayout();
    mApertureFloorSlider = new QSlider(Qt::Horizontal, group);
    mApertureFloorSlider->setObjectName("ApertureFloorSlider");
    mApertureFloorSlider->setRange(Settings::KApertureLimitsDecibels.first,
                                   Settings::KApertureLimitsDecibels.second);
    mApertureFloorSlider->setValue(static_cast<int>(mSettings.GetApertureFloorDecibels()));
    mApertureFloorLabel =
      new QLabel(QString::number(static_cast<int>(mSettings.GetApertureFloorDecibels())), group);
    mApertureFloorLabel->setObjectName("ApertureFloorLabel");
    mApertureFloorLabel->setMinimumWidth(KLabelMinWidth);
    floorLayout->addWidget(mApertureFloorSlider);
    floorLayout->addWidget(mApertureFloorLabel);
    formLayout->addRow("Aperture Floor:", floorLayout);

    connect(mApertureFloorSlider, &QSlider::valueChanged, this, [this](int aValue) {
        mApertureFloorLabel->setText(QString::number(aValue));
        mSettings.SetApertureFloorDecibels(static_cast<float>(aValue));
    });

    // Aperture ceiling slider
    auto* ceilingLayout = new QHBoxLayout();
    mApertureCeilingSlider = new QSlider(Qt::Horizontal, group);
    mApertureCeilingSlider->setObjectName("ApertureCeilingSlider");
    mApertureCeilingSlider->setRange(Settings::KApertureLimitsDecibels.first,
                                     Settings::KApertureLimitsDecibels.second);
    mApertureCeilingSlider->setValue(static_cast<int>(mSettings.GetApertureCeilingDecibels()));
    mApertureCeilingLabel =
      new QLabel(QString::number(static_cast<int>(mSettings.GetApertureCeilingDecibels())), group);
    mApertureCeilingLabel->setObjectName("ApertureCeilingLabel");
    mApertureCeilingLabel->setMinimumWidth(KLabelMinWidth);
    ceilingLayout->addWidget(mApertureCeilingSlider);
    ceilingLayout->addWidget(mApertureCeilingLabel);
    formLayout->addRow("Aperture Ceiling:", ceilingLayout);

    connect(mApertureCeilingSlider, &QSlider::valueChanged, this, [this](int aValue) {
        mApertureCeilingLabel->setText(QString::number(aValue));
        mSettings.SetApertureCeilingDecibels(static_cast<float>(aValue));
    });

    return group;
}

QGroupBox*
SettingsPanel::CreateColorMapControlsGroup()
{
    auto* group = new QGroupBox("Color Map", this);
    group->setObjectName("ColorMapControlsGroup");
    auto* formLayout = new QFormLayout(group);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // Assign member now so UpdateColorMapDropdowns can access it
    mColorMapControlsGroup = group;

    // Create combo boxes for all possible channels (initially hidden)
    for (ChannelCount i = 0; i < GKMaxChannels; ++i) {
        mColorMapComboBoxes.at(i) = CreateColorMapComboBox(i);
        formLayout->addRow(QString("Color Map %1:").arg(i + 1), mColorMapComboBoxes.at(i));
        mColorMapComboBoxes.at(i)->setVisible(false);
    }

    // Default to showing 2 color maps (stereo)
    UpdateColorMapDropdowns(2);

    return group;
}

QGroupBox*
SettingsPanel::CreateDisplayControlsGroup()
{
    auto* group = new QGroupBox("Display", this);
    group->setObjectName("DisplayControlsGroup");
    auto* layout = new QVBoxLayout(group);

    mLiveModeButton = new QPushButton("Live Mode", group);
    mLiveModeButton->setObjectName("LiveModeButton");
    layout->addWidget(mLiveModeButton);

    connect(
      mLiveModeButton, &QPushButton::clicked, this, [this]() { mSettings.SetLiveMode(true); });

    return group;
}

void
SettingsPanel::PopulateAudioDevices()
{
    mAudioDeviceComboBox->clear();

    const auto devices = mSettingsController.GetAudioInputs();
    for (const auto& device : devices) {
        mAudioDeviceComboBox->addItem(device->Description(), QVariant::fromValue(device->Id()));
    }

    // Select default device
    const auto defaultDeviceId = mSettingsController.GetDefaultAudioInput()->Id();
    const int defaultIndex = mAudioDeviceComboBox->findData(QVariant::fromValue(defaultDeviceId));
    if (defaultIndex >= 0) {
        mAudioDeviceComboBox->setCurrentIndex(defaultIndex);
    }
}

void
SettingsPanel::PopulateSampleRates()
{
    mSampleRateComboBox->clear();

    if (mAudioDeviceComboBox->currentIndex() < 0) {
        return;
    }

    const auto deviceId = mAudioDeviceComboBox->currentData().toByteArray();
    const auto supportedRates = mSettingsController.GetSupportedSampleRates(deviceId);

    if (supportedRates.has_value()) {
        for (const auto rate : supportedRates.value()) {
            mSampleRateComboBox->addItem(QString("%1 Hz").arg(rate), rate);
        }
    }

    // Default to 44100 if available
    const int defaultIndex = mSampleRateComboBox->findData(44100);
    if (defaultIndex >= 0) {
        mSampleRateComboBox->setCurrentIndex(defaultIndex);
    }
}

void
SettingsPanel::PopulateChannels()
{
    mChannelsComboBox->clear();

    if (mAudioDeviceComboBox->currentIndex() < 0) {
        return;
    }

    const auto deviceId = mAudioDeviceComboBox->currentData().toByteArray();
    const auto supportedChannels = mSettingsController.GetSupportedChannels(deviceId);

    if (supportedChannels.has_value()) {
        for (const auto channel : supportedChannels.value()) {
            mChannelsComboBox->addItem(QString::number(channel), channel);
        }
    }

    // Default to 2 channels (stereo) if available
    const int defaultIndex = mChannelsComboBox->findData(2);
    if (defaultIndex >= 0) {
        mChannelsComboBox->setCurrentIndex(defaultIndex);
    }
}

void
SettingsPanel::OnRecordingStateChanged(bool aIsRecording)
{
    mAudioDeviceComboBox->setEnabled(!aIsRecording);
    mSampleRateComboBox->setEnabled(!aIsRecording);
    mChannelsComboBox->setEnabled(!aIsRecording);
    mOpenFileButton->setEnabled(!aIsRecording);
    mRecordButton->setText(aIsRecording ? "Stop Recording" : "Start Recording");
}

void
SettingsPanel::ToggleRecording()
{
    if (mSettingsController.IsRecording()) {
        mSettingsController.StopRecording();
    } else {
        const auto deviceId = mAudioDeviceComboBox->currentData().toByteArray();
        const auto sampleRate = mSampleRateComboBox->currentData().toInt();
        const auto channels = static_cast<ChannelCount>(mChannelsComboBox->currentData().toInt());

        (void)mSettingsController.StartRecording(deviceId, channels, sampleRate);
    }
}

void
SettingsPanel::OpenFile()
{
    const QString fileName = QFileDialog::getOpenFileName(
      this,
      "Open Audio File",
      QString(),
      "Audio Files (*.wav *.flac *.ogg *.mp3 *.aiff *.aif);;All Files (*)");

    if (fileName.isEmpty()) {
        return;
    }

    QProgressDialog progress("Loading audio file...", "Cancel", 0, KProgressMaxPercent, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);

    AudioFileReader reader(fileName.toStdString());

    // Update colormap dropdowns based on file channel count
    UpdateColorMapDropdowns(reader.GetChannelCount());

    mAudioFile.LoadFile(reader, [&progress](int aPercent) {
        progress.setValue(aPercent);
        // Allow event processing so cancel button works
        QCoreApplication::processEvents();
    });

    progress.close();
}

QComboBox*
SettingsPanel::CreateColorMapComboBox(ChannelCount aChannel)
{
    auto* comboBox = new QComboBox(this);
    comboBox->setObjectName(QString("ColorMapComboBox%1").arg(aChannel));
    comboBox->setIconSize(QSize(KColorMapIconWidth, KColorMapIconHeight));

    // Add all color map types with preview icons
    for (const auto& [type, name] : ColorMap::TypeNames) {
        comboBox->addItem(QIcon(ColorMap::GeneratePreview(type)),
                          QString::fromUtf8(name.data(), static_cast<int>(name.size())),
                          static_cast<int>(type));
    }

    // Set default selection
    const int defaultIndex =
      comboBox->findData(static_cast<int>(mSettings.GetColorMapType(aChannel)));
    if (defaultIndex >= 0) {
        comboBox->setCurrentIndex(defaultIndex);
    }

    // Connect selection changes
    connect(comboBox, &QComboBox::currentIndexChanged, this, [this, aChannel, comboBox]() {
        const auto type = static_cast<ColorMap::Type>(comboBox->currentData().toInt());
        mSettings.SetColorMapType(aChannel, type);
    });

    return comboBox;
}

QComboBox*
SettingsPanel::GetColorMapComboBox(ChannelCount aChannel) const
{
    if (aChannel >= GKMaxChannels) {
        throw std::out_of_range("Channel index out of range");
    }
    return mColorMapComboBoxes.at(aChannel);
}

void
SettingsPanel::UpdateColorMapDropdowns(ChannelCount aChannelCount)
{
    const auto clampedCount = std::min(aChannelCount, GKMaxChannels);

    // Show/hide combo boxes based on channel count
    for (ChannelCount i = 0; i < GKMaxChannels; ++i) {
        const bool visible = i < clampedCount;
        mColorMapComboBoxes.at(i)->setVisible(visible);

        // Also hide the label in the form layout
        if (auto* formLayout = qobject_cast<QFormLayout*>(mColorMapControlsGroup->layout())) {
            if (QWidget* label = formLayout->labelForField(mColorMapComboBoxes.at(i))) {
                label->setVisible(visible);
            }
        }
    }

    mVisibleColorMaps = clampedCount;
}
