#include "settings_panel.h"
#include "fft_window.h"
#include "include/global_constants.h"
#include "models/audio_recorder.h"
#include "models/settings.h"
#include <QAudioDevice>
#include <QComboBox>
#include <QFormLayout>
#include <QImage>
#include <QLabel>
#include <QMediaDevices>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>
#include <Qt>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <qrgb.h>
#include <string>
#include <string_view>
#include <utility>

SettingsPanel::SettingsPanel(Settings& aSettings, AudioRecorder& aAudioRecorder, QWidget* parent)
  : QWidget(parent)
  , mSettings(&aSettings)
  , mAudioRecorder(&aAudioRecorder)
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
    const std::array<size_t, 5> fftSizes{ 512, 1024, 2048, 4096, 8192 };
    for (const auto size : fftSizes) {
        mFFTSizeCombo->addItem(QString::number(size), QVariant::fromValue(size));
    }

    // Set initial value
    const int currentIndex = mFFTSizeCombo->findData(QVariant::fromValue(mSettings->GetFFTSize()));
    if (currentIndex >= 0) {
        mFFTSizeCombo->setCurrentIndex(currentIndex);
    }

    // Connect to settings
    connect(mFFTSizeCombo, &QComboBox::currentIndexChanged, this, [this](int /*aIndex*/) {
        const size_t selectedSize = mFFTSizeCombo->currentData().toULongLong();
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
    const std::array<size_t, 5> scaleValues{ 1, 2, 4, 8, 16 };
    const size_t currentScale = mSettings->GetWindowScale();
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
        const std::array<size_t, 5> scaleValues{ 1, 2, 4, 8, 16 };
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
    constexpr int kApertureMinValue = -80;
    constexpr int kApertureMaxValue = 30;
    constexpr int kApertureTickInterval = 10;

    // Aperture Min
    mApertureMinSlider = new QSlider(Qt::Horizontal, this);
    mApertureMinSlider->setObjectName("apertureMinSlider");
    mApertureMinSlider->setRange(kApertureMinValue, kApertureMaxValue); // -80 to +30 dB
    mApertureMinSlider->setValue(static_cast<int>(mSettings->GetApertureMinDecibels()));
    mApertureMinSlider->setTickPosition(QSlider::TicksBelow);
    mApertureMinSlider->setTickInterval(kApertureTickInterval);

    mApertureMinLabel = new QLabel(this);
    mApertureMinLabel->setObjectName("apertureMinLabel");
    UpdateApertureMinLabel();

    connect(mApertureMinSlider, &QSlider::valueChanged, this, [this](int aValue) {
        mSettings->SetApertureMinDecibels(static_cast<float>(aValue));
        UpdateApertureMinLabel();
    });

    auto* minHBox = new QHBoxLayout();
    minHBox->addWidget(mApertureMinSlider);
    minHBox->addWidget(mApertureMinLabel);

    aLayout->addRow("Aperture Min:", minHBox);

    // Aperture Max
    mApertureMaxSlider = new QSlider(Qt::Horizontal, this);
    mApertureMaxSlider->setObjectName("apertureMaxSlider");
    mApertureMaxSlider->setRange(kApertureMinValue, kApertureMaxValue); // -80 to +30 dB
    mApertureMaxSlider->setValue(static_cast<int>(mSettings->GetApertureMaxDecibels()));
    mApertureMaxSlider->setTickPosition(QSlider::TicksBelow);
    mApertureMaxSlider->setTickInterval(kApertureTickInterval);

    mApertureMaxLabel = new QLabel(this);
    mApertureMaxLabel->setObjectName("apertureMaxLabel");
    UpdateApertureMaxLabel();

    connect(mApertureMaxSlider, &QSlider::valueChanged, this, [this](int aValue) {
        mSettings->SetApertureMaxDecibels(static_cast<float>(aValue));
        UpdateApertureMaxLabel();
    });

    auto* maxHBox = new QHBoxLayout();
    maxHBox->addWidget(mApertureMaxSlider);
    maxHBox->addWidget(mApertureMaxLabel);

    aLayout->addRow("Aperture Max:", maxHBox);
}

void
SettingsPanel::CreateColorMapControls(QFormLayout* aLayout)
{
    // Only include implemented color maps (not Viridis, Plasma, Inferno, Magma yet)
    static const std::array<std::pair<Settings::ColorMapType, std::string_view>, 7>
      ImplementedColorMaps = { { { Settings::ColorMapType::White, "White" },
                                 { Settings::ColorMapType::Red, "Red" },
                                 { Settings::ColorMapType::Green, "Green" },
                                 { Settings::ColorMapType::Blue, "Blue" },
                                 { Settings::ColorMapType::Cyan, "Cyan" },
                                 { Settings::ColorMapType::Magenta, "Magenta" },
                                 { Settings::ColorMapType::Yellow, "Yellow" } } };

    constexpr int kPreviewIconWidth = 128;
    constexpr int kPreviewIconHeight = 16;

    for (size_t i = 0; i < KNumColorMapSelectors; i++) {
        auto* combo = new QComboBox(this);
        combo->setObjectName(QString("colorMapCombo%1").arg(i));
        combo->setIconSize(QSize(kPreviewIconWidth, kPreviewIconHeight));

        // Add only implemented color map types with preview icons
        for (const auto& [type, name] : ImplementedColorMaps) {
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
                  static_cast<uint8_t>((pixelX * Settings::KColorMapLUTSize) / kPreviewIconWidth);
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
        if (i < gkMaxChannels) {
            const int currentIndex = combo->findData(static_cast<int>(mSettings->GetColorMap(i)));
            if (currentIndex >= 0) {
                combo->setCurrentIndex(currentIndex);
            }
        }

        // Connect to settings
        const size_t channelIndex = i;
        connect(combo, &QComboBox::currentIndexChanged, this, [this, channelIndex](int /*aIndex*/) {
            if (channelIndex < gkMaxChannels) {
                const auto selectedType = static_cast<Settings::ColorMapType>(
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
SettingsPanel::UpdateApertureMinLabel()
{
    const float value = mSettings->GetApertureMinDecibels();
    mApertureMinLabel->setText(QString::fromStdString(std::format("{:.0f} dB", value)));
}

void
SettingsPanel::UpdateApertureMaxLabel()
{
    const float value = mSettings->GetApertureMaxDecibels();
    mApertureMaxLabel->setText(QString::fromStdString(std::format("{:.0f} dB", value)));
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
    mChannelCountSpinBox->setRange(1, static_cast<int>(gkMaxChannels));
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
    constexpr int kDefaultSampleRate = 44100;
    static const std::array<int, 5> CommonRates = { 22050, 44100, 48000, 88200, 96000 };

    const int minRate = aDevice.minimumSampleRate();
    const int maxRate = aDevice.maximumSampleRate();

    int defaultIndex = 0;
    for (const int rate : CommonRates) {
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
    const int effectiveMax = std::min(actualChannels, static_cast<int>(gkMaxChannels));

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
        const int sampleRate = mSampleRateCombo->currentData().toInt();
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
