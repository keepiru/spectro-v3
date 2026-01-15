#pragma once

#include "models/audio_recorder.h"
#include "models/settings.h"
#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QWidget>
#include <array>

// Forward declarations
class QLabel;
class AudioFile;

/// @brief Configuration panel widget
///
/// Provides controls for adjusting FFT parameters, display settings, and color maps.
class SettingsPanel : public QWidget
{
    Q_OBJECT

  public:
    explicit SettingsPanel(Settings& aSettings,
                           AudioRecorder& aAudioRecorder,
                           AudioFile& aAudioFile,
                           QWidget* parent = nullptr);
    ~SettingsPanel() override = default;

  private:
    Settings* mSettings = nullptr;
    AudioFile* mAudioFile = nullptr;
    AudioRecorder* mAudioRecorder = nullptr;

    // Control widgets
    QComboBox* mWindowTypeCombo = nullptr;
    QComboBox* mFFTSizeCombo = nullptr;
    QSlider* mWindowScaleSlider = nullptr;
    QLabel* mWindowScaleLabel = nullptr;
    QSlider* mApertureMinSlider = nullptr;
    QLabel* mApertureMinLabel = nullptr;
    QSlider* mApertureMaxSlider = nullptr;
    QLabel* mApertureMaxLabel = nullptr;

    static constexpr size_t KNumColorMapSelectors = 6;
    std::array<QComboBox*, KNumColorMapSelectors> mColorMapCombos = {};

    // Audio controls
    QComboBox* mAudioDeviceCombo = nullptr;
    QComboBox* mSampleRateCombo = nullptr;
    QSpinBox* mChannelCountSpinBox = nullptr;
    QPushButton* mRecordingButton = nullptr;

    // File controls
    QPushButton* mOpenFileButton = nullptr;

    // Helper methods
    void CreateLayout();
    void CreateAudioControls(class QFormLayout* aLayout);
    void CreateOpenFileButton(class QFormLayout* aLayout);
    void CreateWindowTypeControl(class QFormLayout* aLayout);
    void CreateFFTSizeControl(class QFormLayout* aLayout);
    void CreateWindowScaleControl(class QFormLayout* aLayout);
    void CreateApertureControls(class QFormLayout* aLayout);
    void CreateColorMapControls(class QFormLayout* aLayout);
    void UpdateWindowScaleLabel();
    void UpdateApertureMinLabel();
    void UpdateApertureMaxLabel();
    void UpdateRecordingControlsEnabled(bool aIsRecording);
    void UpdateSampleRatesForDevice(const QAudioDevice& aDevice);
    void UpdateChannelRangeForDevice(const QAudioDevice& aDevice);
    void OnRecordingButtonClicked();
    void OnRecordingStateChanged(bool aIsRecording);
    void OnOpenFileClicked();
};
