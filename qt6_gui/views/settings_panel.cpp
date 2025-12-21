#include "settings_panel.h"
#include "include/global_constants.h"
#include "models/settings.h"
#include <QComboBox>
#include <QFormLayout>
#include <QImage>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>
#include <Qt>
#include <cstdint>
#include <format>
#include <string>

SettingsPanel::SettingsPanel(Settings& aSettings, AudioRecorder& aAudioRecorder, QWidget* parent)
  : QWidget(parent)
  , mSettings(&aSettings)
  , mAudioRecorder(&aAudioRecorder)
{
    constexpr int kPanelWidth = 300;
    setFixedWidth(kPanelWidth);

    CreateLayout();
}

void
SettingsPanel::CreateLayout()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(8);

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
    // Aperture Min
    mApertureMinSlider = new QSlider(Qt::Horizontal, this);
    mApertureMinSlider->setObjectName("apertureMinSlider");
    mApertureMinSlider->setRange(-80, 30); // -80 to +30 dB
    mApertureMinSlider->setValue(static_cast<int>(mSettings->GetApertureMinDecibels()));
    mApertureMinSlider->setTickPosition(QSlider::TicksBelow);
    mApertureMinSlider->setTickInterval(10);

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
    mApertureMaxSlider->setRange(-80, 30); // -80 to +30 dB
    mApertureMaxSlider->setValue(static_cast<int>(mSettings->GetApertureMaxDecibels()));
    mApertureMaxSlider->setTickPosition(QSlider::TicksBelow);
    mApertureMaxSlider->setTickInterval(10);

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
      implementedColorMaps = { { { Settings::ColorMapType::White, "White" },
                                 { Settings::ColorMapType::Red, "Red" },
                                 { Settings::ColorMapType::Green, "Green" },
                                 { Settings::ColorMapType::Blue, "Blue" },
                                 { Settings::ColorMapType::Cyan, "Cyan" },
                                 { Settings::ColorMapType::Magenta, "Magenta" },
                                 { Settings::ColorMapType::Yellow, "Yellow" } } };

    for (size_t i = 0; i < KNumColorMapSelectors; i++) {
        auto* combo = new QComboBox(this);
        combo->setObjectName(QString("colorMapCombo%1").arg(i));
        combo->setIconSize(QSize(128, 16));

        // Add only implemented color map types with preview icons
        for (const auto& [type, name] : implementedColorMaps) {
            // Create a preview image for this color map
            constexpr int previewWidth = 128;
            constexpr int previewHeight = 16;
            QImage preview(previewWidth, previewHeight, QImage::Format_RGB888);

            // Get a temporary copy of the color map LUT
            Settings tempSettings;
            tempSettings.SetColorMap(0, type);
            const auto& lut = tempSettings.GetColorMapLUTs().at(0);

            // Fill the preview image
            for (int x = 0; x < previewWidth; x++) {
                // Map x to LUT index (0-255)
                const auto lutIndex =
                  static_cast<uint8_t>((x * Settings::KColorMapLUTSize) / previewWidth);
                const auto& color = lut.at(lutIndex);

                for (int y = 0; y < previewHeight; y++) {
                    preview.setPixel(x, y, qRgb(color.r, color.g, color.b));
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
