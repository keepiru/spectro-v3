// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "settings_panel.h"
#include "controllers/audio_file.h"
#include "controllers/audio_file_reader.h"
#include "controllers/audio_recorder.h"
#include "fft_window.h"
#include "include/global_constants.h"
#include "models/colormap.h"
#include "models/settings.h"
#include <QAudioDevice>
#include <QComboBox>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFormLayout>
#include <QImage>
#include <QLabel>
#include <QMediaDevices>
#include <QMessageBox>
#include <QObject>
#include <QProgressDialog>
#include <QPushButton>
#include <QRgb>
#include <QSize>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>
#include <Qt>
#include <algorithm>
#include <array>
#include <audio_types.h>
#include <cstddef>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>

namespace {
constexpr int KProgressMaximum = 100;
} // namespace

SettingsPanel::SettingsPanel(Settings& aSettings,
                             AudioRecorder& aAudioRecorder,
                             AudioFile& aAudioFile,
                             QWidget* parent)
  : QWidget(parent)
  , mSettings(&aSettings)
  , mAudioRecorder(&aAudioRecorder)
  , mAudioFile(&aAudioFile)
{
    constexpr int kPanelWidth = 300;
    setFixedWidth(kPanelWidth);

    CreateLayout();

    // Connect to audio recorder state changes
    connect(mAudioRecorder,
            &AudioRecorder::RecordingStateChanged,
            this,
            &SettingsPanel::OnRecordingStateChanged);

    // Set initial state based on current recording status
    OnRecordingStateChanged(mAudioRecorder->IsRecording());
}

void
SettingsPanel::CreateLayout()
{
    constexpr int kPanelMargin = 10;
    constexpr int kPanelSpacing = 10;
    constexpr int kFormSpacing = 8;

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(kPanelMargin, kPanelMargin, kPanelMargin, kPanelMargin);
    layout->setSpacing(kPanelSpacing);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(kFormSpacing);

    CreateAudioControls(formLayout);
    CreateOpenFileButton(formLayout);
    CreateLiveModeButton(formLayout);
    CreateWindowTypeControl(formLayout);
    CreateFFTSizeControl(formLayout);
    CreateWindowScaleControl(formLayout);
    CreateApertureControls(formLayout);
    CreateColorMapControls(formLayout);

    layout->addLayout(formLayout);
    layout->addStretch();
}

void
SettingsPanel::CreateWindowTypeControl(QFormLayout* aLayout)
{
    mWindowTypeCombo = new QComboBox(this);
    mWindowTypeCombo->setObjectName("windowTypeCombo");
    mWindowTypeCombo->addItem("Rectangular", static_cast<int>(FFTWindow::Type::Rectangular));
    mWindowTypeCombo->addItem("Hann", static_cast<int>(FFTWindow::Type::Hann));
    mWindowTypeCombo->addItem("Hamming", static_cast<int>(FFTWindow::Type::Hamming));
    mWindowTypeCombo->addItem("Blackman", static_cast<int>(FFTWindow::Type::Blackman));
    mWindowTypeCombo->addItem("Blackman-Harris", static_cast<int>(FFTWindow::Type::BlackmanHarris));

    // Set initial value
    const int currentIndex =
      mWindowTypeCombo->findData(static_cast<int>(mSettings->GetWindowType()));
    if (currentIndex >= 0) {
        mWindowTypeCombo->setCurrentIndex(currentIndex);
    }

    // Connect to settings
    connect(mWindowTypeCombo, &QComboBox::currentIndexChanged, this, [this](int /*aIndex*/) {
        const auto selectedType =
          static_cast<FFTWindow::Type>(mWindowTypeCombo->currentData().toInt());
        mSettings->SetFFTSettings(mSettings->GetFFTSize(), selectedType);
    });

    aLayout->addRow("Window Type:", mWindowTypeCombo);
}

void
SettingsPanel::CreateFFTSizeControl(QFormLayout* aLayout)
{
    mFFTSizeCombo = new QComboBox(this);
    mFFTSizeCombo->setObjectName("fftSizeCombo");

    // Add FFT size options
    for (const auto size : Settings::KValidFFTSizes) {
        mFFTSizeCombo->addItem(QString::number(size), static_cast<int>(size));
    }

    // Set initial value
    const FFTSize kCurrentFFTSize = mSettings->GetFFTSize();
    const int currentIndex = mFFTSizeCombo->findData(static_cast<int>(kCurrentFFTSize));
    if (currentIndex >= 0) {
        mFFTSizeCombo->setCurrentIndex(currentIndex);
    }

    // Connect to settings
    connect(mFFTSizeCombo, &QComboBox::currentIndexChanged, this, [this](int aIndex) {
        if (aIndex < 0) {
            return; // No valid selection yet
        }
        const int sizeInt = mFFTSizeCombo->currentData().toInt();
        if (sizeInt == 0) {
            return; // Invalid data
        }
        const FFTSize selectedSize = sizeInt;
        mSettings->SetFFTSettings(selectedSize, mSettings->GetWindowType());
    });

    aLayout->addRow("FFT Size:", mFFTSizeCombo);
}

void
SettingsPanel::CreateWindowScaleControl(QFormLayout* aLayout)
{
    mWindowScaleSlider = new QSlider(Qt::Horizontal, this);
    mWindowScaleSlider->setObjectName("windowScaleSlider");
    mWindowScaleSlider->setRange(0, 4); // 0=1, 1=2, 2=4, 3=8, 4=16
    mWindowScaleSlider->setTickPosition(QSlider::TicksBelow);
    mWindowScaleSlider->setTickInterval(1);

    // Set initial value
    const std::array<WindowScale, 5> scaleValues{ 1, 2, 4, 8, 16 };
    const WindowScale currentScale = mSettings->GetWindowScale();
    for (size_t i = 0; i < scaleValues.size(); i++) {
        if (scaleValues.at(i) == currentScale) {
            mWindowScaleSlider->setValue(static_cast<int>(i));
            break;
        }
    }

    mWindowScaleLabel = new QLabel(this);
    mWindowScaleLabel->setObjectName("windowScaleLabel");
    UpdateWindowScaleLabel();

    // Connect to settings
    connect(mWindowScaleSlider, &QSlider::valueChanged, this, [this](int aValue) {
        const std::array<WindowScale, 5> scaleValues{ 1, 2, 4, 8, 16 };
        mSettings->SetWindowScale(scaleValues.at(static_cast<size_t>(aValue)));
        UpdateWindowScaleLabel();
    });

    auto* hbox = new QHBoxLayout();
    hbox->addWidget(mWindowScaleSlider);
    hbox->addWidget(mWindowScaleLabel);

    aLayout->addRow("Window Scale:", hbox);
}

void
SettingsPanel::CreateApertureControls(QFormLayout* aLayout)
{
    constexpr int kApertureTickInterval = 10;

    // Aperture Floor
    mApertureFloorSlider = new QSlider(Qt::Horizontal, this);
    mApertureFloorSlider->setObjectName("apertureFloorSlider");
    mApertureFloorSlider->setRange(Settings::KApertureLimitsDecibels.first,
                                   Settings::KApertureLimitsDecibels.second);
    mApertureFloorSlider->setValue(static_cast<int>(mSettings->GetApertureFloorDecibels()));
    mApertureFloorSlider->setTickPosition(QSlider::TicksBelow);
    mApertureFloorSlider->setTickInterval(kApertureTickInterval);

    mApertureFloorLabel = new QLabel(this);
    mApertureFloorLabel->setObjectName("apertureFloorLabel");
    UpdateApertureFloorLabel();

    connect(mApertureFloorSlider, &QSlider::valueChanged, this, [this](int aValue) {
        mSettings->SetApertureFloorDecibels(static_cast<float>(aValue));
        UpdateApertureFloorLabel();
    });

    auto* floorHBox = new QHBoxLayout();
    floorHBox->addWidget(mApertureFloorSlider);
    floorHBox->addWidget(mApertureFloorLabel);

    aLayout->addRow("Aperture Floor:", floorHBox);
    // Aperture Ceiling
    mApertureCeilingSlider = new QSlider(Qt::Horizontal, this);
    mApertureCeilingSlider->setObjectName("apertureCeilingSlider");
    mApertureCeilingSlider->setRange(Settings::KApertureLimitsDecibels.first,
                                     Settings::KApertureLimitsDecibels.second);
    mApertureCeilingSlider->setValue(static_cast<int>(mSettings->GetApertureCeilingDecibels()));
    mApertureCeilingSlider->setTickPosition(QSlider::TicksBelow);
    mApertureCeilingSlider->setTickInterval(kApertureTickInterval);

    mApertureCeilingLabel = new QLabel(this);
    mApertureCeilingLabel->setObjectName("apertureCeilingLabel");
    UpdateApertureCeilingLabel();

    connect(mApertureCeilingSlider, &QSlider::valueChanged, this, [this](int aValue) {
        mSettings->SetApertureCeilingDecibels(static_cast<float>(aValue));
        UpdateApertureCeilingLabel();
    });

    auto* ceilingHBox = new QHBoxLayout();
    ceilingHBox->addWidget(mApertureCeilingSlider);
    ceilingHBox->addWidget(mApertureCeilingLabel);

    aLayout->addRow("Aperture Ceiling:", ceilingHBox);
}

void
SettingsPanel::CreateColorMapControls(QFormLayout* aLayout)
{
    constexpr int kPreviewIconWidth = 128;
    constexpr int kPreviewIconHeight = 16;

    for (size_t i = 0; i < KNumColorMapSelectors; i++) {
        auto* combo = new QComboBox(this);
        combo->setObjectName(QString("colorMapCombo%1").arg(i));
        combo->setIconSize(QSize(kPreviewIconWidth, kPreviewIconHeight));

        // Add color map types with preview icons
        for (const auto& [type, name] : ColorMap::TypeNames) {
            // Create a preview image for this color map
            QImage preview(kPreviewIconWidth, kPreviewIconHeight, QImage::Format_RGB888);

            // Get a temporary copy of the color map LUT
            Settings tempSettings;
            tempSettings.SetColorMap(0, type);
            const auto& lut = tempSettings.GetColorMapLUTs().at(0);

            // Fill the preview image
            for (int pixelX = 0; pixelX < kPreviewIconWidth; pixelX++) {
                // Map x to LUT index (0-255)
                const auto lutIndex =
                  static_cast<uint8_t>((pixelX * ColorMap::KLUTSize) / kPreviewIconWidth);
                const auto& color = lut.at(lutIndex);

                for (int pixelY = 0; pixelY < kPreviewIconHeight; pixelY++) {
                    preview.setPixel(pixelX, pixelY, qRgb(color.r, color.g, color.b));
                }
            }

            combo->addItem(QPixmap::fromImage(preview),
                           QString::fromStdString(std::string(name)),
                           static_cast<int>(type));
        }

        // Set initial value if within channel range
        if (i < GKMaxChannels) {
            const int currentIndex = combo->findData(static_cast<int>(mSettings->GetColorMap(i)));
            if (currentIndex >= 0) {
                combo->setCurrentIndex(currentIndex);
            }
        }

        // Connect to settings
        const size_t channelIndex = i;
        connect(combo, &QComboBox::currentIndexChanged, this, [this, channelIndex](int /*aIndex*/) {
            if (channelIndex < GKMaxChannels) {
                const auto selectedType = static_cast<ColorMap::Type>(
                  mColorMapCombos.at(channelIndex)->currentData().toInt());
                mSettings->SetColorMap(channelIndex, selectedType);
            }
        });

        mColorMapCombos.at(i) = combo;
        aLayout->addRow(QString("Color Map %1:").arg(i + 1), combo);
    }
}

void
SettingsPanel::UpdateWindowScaleLabel()
{
    const size_t scale = mSettings->GetWindowScale();
    mWindowScaleLabel->setText(QString::number(scale));
}

void
SettingsPanel::UpdateApertureFloorLabel()
{
    const float value = mSettings->GetApertureFloorDecibels();
    mApertureFloorLabel->setText(QString::fromStdString(std::format("{:.0f} dB", value)));
}

void
SettingsPanel::UpdateApertureCeilingLabel()
{
    const float value = mSettings->GetApertureCeilingDecibels();
    mApertureCeilingLabel->setText(QString::fromStdString(std::format("{:.0f} dB", value)));
}

void
SettingsPanel::CreateAudioControls(QFormLayout* aLayout)
{
    // Audio Device selection
    mAudioDeviceCombo = new QComboBox(this);
    mAudioDeviceCombo->setObjectName("audioDeviceCombo");

    const auto audioInputs = QMediaDevices::audioInputs();
    const auto defaultDevice = QMediaDevices::defaultAudioInput();
    int defaultIndex = 0;

    for (int i = 0; i < audioInputs.size(); ++i) {
        const auto& device = audioInputs.at(i);
        mAudioDeviceCombo->addItem(device.description(), QVariant::fromValue(device));
        if (device == defaultDevice) {
            defaultIndex = i;
        }
    }
    mAudioDeviceCombo->setCurrentIndex(defaultIndex);

    // Update sample rates and channel range when device changes
    connect(mAudioDeviceCombo, &QComboBox::currentIndexChanged, this, [this](int /*aIndex*/) {
        const auto device = mAudioDeviceCombo->currentData().value<QAudioDevice>();
        UpdateSampleRatesForDevice(device);
        UpdateChannelRangeForDevice(device);
    });

    aLayout->addRow("Audio Device:", mAudioDeviceCombo);

    // Sample Rate selection
    mSampleRateCombo = new QComboBox(this);
    mSampleRateCombo->setObjectName("sampleRateCombo");

    // Populate with rates from default device initially
    UpdateSampleRatesForDevice(defaultDevice);

    aLayout->addRow("Sample Rate:", mSampleRateCombo);

    // Channel Count selection
    mChannelCountSpinBox = new QSpinBox(this);
    mChannelCountSpinBox->setObjectName("channelCountSpinBox");
    mChannelCountSpinBox->setRange(1, static_cast<int>(GKMaxChannels));
    mChannelCountSpinBox->setValue(2); // Default to stereo

    // Update range based on default device capabilities
    UpdateChannelRangeForDevice(defaultDevice);

    aLayout->addRow("Channels:", mChannelCountSpinBox);

    // Start/Stop Recording button
    mRecordingButton = new QPushButton("Start Recording", this);
    mRecordingButton->setObjectName("recordingButton");

    connect(
      mRecordingButton, &QPushButton::clicked, this, &SettingsPanel::OnRecordingButtonClicked);

    aLayout->addRow("", mRecordingButton);
}

void
SettingsPanel::UpdateSampleRatesForDevice(const QAudioDevice& aDevice)
{
    mSampleRateCombo->clear();

    // Common sample rates to offer
    constexpr SampleRate kDefaultSampleRate = 44100;
    static const std::array<SampleRate, 5> CommonRates = { 22050, 44100, 48000, 88200, 96000 };

    const SampleRate minRate = aDevice.minimumSampleRate();
    const SampleRate maxRate = aDevice.maximumSampleRate();

    int defaultIndex = 0;
    for (const SampleRate rate : CommonRates) {
        if (rate >= minRate && rate <= maxRate) {
            mSampleRateCombo->addItem(QString::number(rate) + " Hz", rate);
            if (rate == kDefaultSampleRate) {
                defaultIndex = mSampleRateCombo->count() - 1;
            }
        }
    }

    mSampleRateCombo->setCurrentIndex(defaultIndex);
}

void
SettingsPanel::UpdateChannelRangeForDevice(const QAudioDevice& aDevice)
{
    // Use the device's preferred format to get the actual channel count
    // maximumChannelCount() returns theoretical max, not hardware reality
    const auto preferredFormat = aDevice.preferredFormat();
    const int actualChannels = preferredFormat.channelCount();

    // Allow from 1 up to the device's actual channel count, clamped to our app max
    const int effectiveMax = std::min(actualChannels, static_cast<int>(GKMaxChannels));

    // Preserve current value if possible, otherwise clamp it
    const int currentValue = mChannelCountSpinBox->value();
    mChannelCountSpinBox->setRange(1, effectiveMax);

    // Restore value or clamp to new range
    if (currentValue > effectiveMax) {
        mChannelCountSpinBox->setValue(effectiveMax);
    } else if (currentValue < 1) {
        mChannelCountSpinBox->setValue(1);
    } else {
        mChannelCountSpinBox->setValue(currentValue);
    }
}

void
SettingsPanel::UpdateRecordingControlsEnabled(bool aIsRecording)
{
    // Disable device/rate/channels controls while recording
    mAudioDeviceCombo->setEnabled(!aIsRecording);
    mSampleRateCombo->setEnabled(!aIsRecording);
    mChannelCountSpinBox->setEnabled(!aIsRecording);
}

void
SettingsPanel::OnRecordingButtonClicked()
{
    if (mAudioRecorder->IsRecording()) {
        mAudioRecorder->Stop();
    } else {
        const auto device = mAudioDeviceCombo->currentData().value<QAudioDevice>();
        const SampleRate sampleRate = mSampleRateCombo->currentData().toInt();
        const int channelCount = mChannelCountSpinBox->value();

        mAudioRecorder->Start(device, channelCount, sampleRate);
    }
}

void
SettingsPanel::OnRecordingStateChanged(bool aIsRecording)
{
    mRecordingButton->setText(aIsRecording ? "Stop Recording" : "Start Recording");
    UpdateRecordingControlsEnabled(aIsRecording);
}

void
SettingsPanel::CreateOpenFileButton(QFormLayout* aLayout)
{
    mOpenFileButton = new QPushButton("Open File", this);
    mOpenFileButton->setObjectName("openFileButton");

    connect(mOpenFileButton, &QPushButton::clicked, this, &SettingsPanel::OnOpenFileClicked);

    aLayout->addRow("", mOpenFileButton);
}

void
SettingsPanel::OnOpenFileClicked()
{
    // Open file dialog to choose an audio file
    const QString fileName =
      QFileDialog::getOpenFileName(this,
                                   tr("Open Audio File"),
                                   QString(),
                                   tr("Audio Files (*.wav *.aiff *.flac *.ogg);;All Files (*)"));

    if (fileName.isEmpty()) {
        return; // User canceled
    }

    // Create progress dialog
    QProgressDialog progressDialog("Loading audio file...", "Cancel", 0, KProgressMaximum, this);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setMinimumDuration(0); // Show immediately

    // Create progress callback
    auto progressCallback = [&progressDialog](int aProgressPercent) {
        progressDialog.setValue(aProgressPercent);
    };

    // Load the file
    try {
        AudioFileReader reader(fileName.toStdString());
        const bool success = mAudioFile->LoadFile(reader, progressCallback);

        if (!success) {
            QMessageBox::warning(this, "Error", "Failed to load audio file.");
        }
    } catch (const std::runtime_error& e) {
        QMessageBox::critical(
          this, "Error", QString("Failed to load audio file: %1").arg(e.what()));
    }

    progressDialog.setValue(KProgressMaximum);
}

void
SettingsPanel::CreateLiveModeButton(QFormLayout* aLayout)
{
    mLiveModeButton = new QPushButton("Live Mode", this);
    mLiveModeButton->setObjectName("liveModeButton");
    connect(
      mLiveModeButton, &QPushButton::clicked, this, [this]() { mSettings->SetLiveMode(true); });
    aLayout->addRow("", mLiveModeButton);
}
