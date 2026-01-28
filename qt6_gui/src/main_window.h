// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include "controllers/audio_file.h"
#include "controllers/audio_recorder.h"
#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "views/scale_view.h"
#include "views/settings_panel.h"
#include "views/spectrogram_view.h"
#include "views/spectrum_plot.h"
#include <QMainWindow>
#include <QWidget>

class Settings;

/// @brief Main application window for Spectro-v3 spectrum analyzer
///
/// Manages the application's UI layout, menus, and coordinate interaction between
/// the audio processing components and visualization widgets.
class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    /// @brief Constructor for MainWindow
    /// @param parent Optional parent widget
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

  private:
    /// @brief Sets up the main layout of the application window
    void CreateLayout();

    /// @brief Sets up signal-slot connections between components
    void SetupConnections();

    /// @brief Applies dark mode theme to the application
    static void SetDarkMode();

    // Models and controllers
    Settings mSettings;
    AudioBuffer mAudioBuffer;
    AudioRecorder mAudioRecorder;
    SpectrogramController mSpectrogramController;
    AudioFile mAudioFile;

    // View widgets
    SpectrogramView mSpectrogramView;
    ScaleView mScaleView;
    SpectrumPlot mSpectrumPlot;
    SettingsPanel mSettingsPanel;
};
