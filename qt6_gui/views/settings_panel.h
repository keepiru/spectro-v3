// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include "include/global_constants.h"
#include <QComboBox>
#include <QGroupBox>
#include <QPushButton>
#include <QSlider>
#include <QWidget>
#include <array>
#include <audio_types.h>

// Forward declarations
class AudioBuffer;
class AudioFile;
class AudioRecorder;
class QAudioDevice;
class QLabel;
class Settings;
class SettingsController;

/// @brief Settings panel widget for controlling audio and display settings
///
/// Provides UI controls for audio device selection, recording, FFT parameters,
/// display aperture, and color map selection. All controls are named for
/// testability.
class SettingsPanel : public QWidget
{
    Q_OBJECT

  public:
    /// @brief Constructor
    /// @param aSettings Reference to Settings model
    /// @param aSettingsController Reference to SettingsController
    /// @param aAudioFile Reference to AudioFile controller
    /// @param aParent Qt parent widget (optional)
    explicit SettingsPanel(Settings& aSettings,
                           SettingsController& aSettingsController,
                           AudioFile& aAudioFile,
                           QWidget* aParent = nullptr);
    ~SettingsPanel() override = default;

    // Test accessors for named elements
    [[nodiscard]] QComboBox* GetAudioDeviceComboBox() const { return mAudioDeviceComboBox; }
    [[nodiscard]] QComboBox* GetSampleRateComboBox() const { return mSampleRateComboBox; }
    [[nodiscard]] QComboBox* GetChannelsComboBox() const { return mChannelsComboBox; }
    [[nodiscard]] QPushButton* GetRecordButton() const { return mRecordButton; }
    [[nodiscard]] QPushButton* GetOpenFileButton() const { return mOpenFileButton; }
    [[nodiscard]] QComboBox* GetWindowTypeComboBox() const { return mWindowTypeComboBox; }
    [[nodiscard]] QComboBox* GetFFTSizeComboBox() const { return mFFTSizeComboBox; }
    [[nodiscard]] QSlider* GetWindowScaleSlider() const { return mWindowScaleSlider; }
    [[nodiscard]] QSlider* GetApertureFloorSlider() const { return mApertureFloorSlider; }
    [[nodiscard]] QSlider* GetApertureCeilingSlider() const { return mApertureCeilingSlider; }
    [[nodiscard]] QLabel* GetWindowScaleLabel() const { return mWindowScaleLabel; }
    [[nodiscard]] QLabel* GetApertureFloorLabel() const { return mApertureFloorLabel; }
    [[nodiscard]] QLabel* GetApertureCeilingLabel() const { return mApertureCeilingLabel; }
    [[nodiscard]] QPushButton* GetLiveModeButton() const { return mLiveModeButton; }
    [[nodiscard]] QComboBox* GetColorMapComboBox(ChannelCount aChannel) const;

    /// @brief Update the number of colormap dropdowns based on channel count
    /// @param aChannelCount Number of channels
    void UpdateColorMapDropdowns(ChannelCount aChannelCount);

    /// @brief Update UI when recording state changes
    /// @param aIsRecording true if now recording, false if stopped
    void OnRecordingStateChanged(bool aIsRecording);

  private:
    /// @brief Create the audio controls group box
    /// @return Pointer to the created group box
    QGroupBox* CreateAudioControlsGroup();

    /// @brief Create the FFT controls group box
    /// @return Pointer to the created group box
    QGroupBox* CreateFFTControlsGroup();

    /// @brief Create the color map controls group box
    /// @return Pointer to the created group box
    QGroupBox* CreateColorMapControlsGroup();

    /// @brief Create the display controls group box
    /// @return Pointer to the created group box
    QGroupBox* CreateDisplayControlsGroup();

    /// @brief Populate audio device combo box from available devices
    void PopulateAudioDevices();

    /// @brief Populate sample rate combo box for the selected device
    void PopulateSampleRates();

    /// @brief Populate channels combo box for the selected device
    void PopulateChannels();

    /// @brief Start or stop recording
    void ToggleRecording();

    /// @brief Open a file dialog and load the selected audio file
    void OpenFile();

    /// @brief Create a colormap combo box with preview icons
    /// @param aChannel Channel index
    /// @return Pointer to the created combo box
    QComboBox* CreateColorMapComboBox(ChannelCount aChannel);

    // Model and controller references
    Settings& mSettings;
    SettingsController& mSettingsController;
    AudioFile& mAudioFile;

    // Audio controls
    QComboBox* mAudioDeviceComboBox = nullptr;
    QComboBox* mSampleRateComboBox = nullptr;
    QComboBox* mChannelsComboBox = nullptr;
    QPushButton* mRecordButton = nullptr;
    QPushButton* mOpenFileButton = nullptr;
    QGroupBox* mAudioControlsGroup = nullptr;

    // FFT controls
    QComboBox* mWindowTypeComboBox = nullptr;
    QComboBox* mFFTSizeComboBox = nullptr;
    QSlider* mWindowScaleSlider = nullptr;
    QLabel* mWindowScaleLabel = nullptr;
    QSlider* mApertureFloorSlider = nullptr;
    QLabel* mApertureFloorLabel = nullptr;
    QSlider* mApertureCeilingSlider = nullptr;
    QLabel* mApertureCeilingLabel = nullptr;

    // Color map controls
    QGroupBox* mColorMapControlsGroup = nullptr;
    std::array<QComboBox*, GKMaxChannels> mColorMapComboBoxes{};
    ChannelCount mVisibleColorMaps = 0;

    // Display controls
    QPushButton* mLiveModeButton = nullptr;
};
