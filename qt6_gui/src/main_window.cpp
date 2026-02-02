// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "main_window.h"
#include "controllers/audio_recorder.h"
#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "models/settings.h"
#include "views/scale_view.h"
#include "views/settings_panel.h"
#include "views/spectrogram_view.h"
#include "views/spectrum_plot.h"
#include <QApplication>
#include <QColor>
#include <QHBoxLayout>
#include <QLoggingCategory>
#include <QMainWindow>
#include <QMediaDevices>
#include <QObject>
#include <QPalette>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWidget>
#include <Qt>
#include <audio_types.h>
#include <cmath>
#include <cstddef>

namespace {
constexpr ChannelCount KDefaultChannelCount = 2;
constexpr SampleRate KDefaultSampleRate = 44100;
}

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , mSettings(this)
  , mAudioBuffer(this)
  , mAudioRecorder(mAudioBuffer, this)
  , mSpectrogramController(mSettings, mAudioBuffer, nullptr, nullptr, this)
  , mAudioFile(mAudioBuffer, this)
  , mSettingsController(mSettings, this)
  , mSpectrogramView(mSpectrogramController, this)
  , mScaleView(mSpectrogramController, this)
  , mSpectrumPlot(mSpectrogramController, this)
  , mSettingsPanel(mSettings, mSettingsController, mAudioRecorder, mAudioFile, this)
{
    constexpr int kDefaultWindowWidth = 1400;
    constexpr int kDefaultWindowHeight = 800;
    constexpr size_t kDefaultStride = 1024;

    setWindowTitle("Spectro-v3 - Real-time Spectrum Analyzer");
    resize(kDefaultWindowWidth, kDefaultWindowHeight);

    // Suppress Qt multimedia FFmpeg logging noise
    QLoggingCategory::setFilterRules("qt.multimedia.ffmpeg=false");

    // For now we don't have a config UI, so just start recording with defaults
    mAudioRecorder.Start(
      QMediaDevices::defaultAudioInput(), KDefaultChannelCount, KDefaultSampleRate);

    SetDarkMode();
    CreateLayout();
    SetupConnections();
}

void
MainWindow::CreateLayout()
{
    // ┌─────────────────────────────────────────────────────────┐
    // │ QHBoxLayout (main)                                      │
    // │ ┌──────────────────────────────┬────────────────────┐   │
    // │ │ QVBoxLayout (left)           │ SettingsPanel      │   │
    // │ │ ┌──────────────────────────┐ │ (~300px)           │   │
    // │ │ │ SpectrogramView          │ │                    │   │
    // │ │ │ (stretch 7)              │ │                    │   │
    // │ │ └──────────────────────────┘ │                    │   │
    // │ │ ┌──────────────────────────┐ │                    │   │
    // │ │ │ ScaleView (fixed 20px)   │ │                    │   │
    // │ │ └──────────────────────────┘ │                    │   │
    // │ │ ┌──────────────────────────┐ │                    │   │
    // │ │ │ SpectrumPlot             │ │                    │   │
    // │ │ │ (stretch 3)              │ │                    │   │
    // │ │ └──────────────────────────┘ │                    │   │
    // │ └──────────────────────────────┴────────────────────┘   │
    // └─────────────────────────────────────────────────────────┘

    // Create left container with vertical layout for the two plots
    auto* leftContainer = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);
    constexpr int kSpectrogramStretch = 7; // 70% of vertical space
    constexpr int kSpectrumStretch = 3;    // 30% of vertical space
    leftLayout->addWidget(&mSpectrogramView, kSpectrogramStretch);
    leftLayout->addWidget(&mScaleView, 0); // Fixed height (no stretch)
    leftLayout->addWidget(&mSpectrumPlot, kSpectrumStretch);

    // Create main horizontal layout
    auto* mainWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(mainWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(leftContainer, 1);   // Takes remaining space
    mainLayout->addWidget(&mSettingsPanel, 0); // Fixed width (no stretch)

    setCentralWidget(mainWidget);
}

void
MainWindow::SetupConnections()
{
    // Future: Connect signals/slots between AudioBuffer, SpectrogramController,
    // and view widgets

    // Update SpectrogramView scrollbar when new audio data is available
    connect(&mAudioBuffer,
            &AudioBuffer::DataAvailable,
            &mSpectrogramView,
            &SpectrogramView::UpdateScrollbarRange);

    // Also update SpectrumPlot
    connect(&mAudioBuffer,
            &AudioBuffer::DataAvailable,
            &mSpectrumPlot,
            qOverload<>(&SpectrumPlot::update));

    // Update views when display settings change
    connect(
      &mSettings, &Settings::DisplaySettingsChanged, &mScaleView, qOverload<>(&ScaleView::update));
    connect(&mSettings,
            &Settings::DisplaySettingsChanged,
            &mSpectrumPlot,
            qOverload<>(&SpectrumPlot::update));
    connect(&mSettings,
            &Settings::DisplaySettingsChanged,
            &mSpectrogramView,
            &SpectrogramView::UpdateViewport);

    // Stop recording when buffer is reset (e.g., when loading a new file)
    connect(&mAudioBuffer, &AudioBuffer::BufferReset, &mAudioRecorder, &AudioRecorder::Stop);

    // Reset scrollbar when buffer is reset
    connect(&mAudioBuffer, &AudioBuffer::BufferReset, [&]() {
        mSpectrogramView.UpdateScrollbarRange(FrameCount(0));
    });

    // Clear live mode when user interacts with scrollbar
    connect(mSpectrogramView.verticalScrollBar(),
            &QScrollBar::actionTriggered,
            &mSettings,
            &Settings::ClearLiveMode);
}

void
MainWindow::SetDarkMode()
{
    // Dark theme color constants
    constexpr QColor kDarkBackground(45, 45, 45);
    constexpr QColor kDarkBase(30, 30, 30);
    constexpr QColor kDarkText(212, 212, 212);
    constexpr QColor kHighlight(42, 130, 218);
    QPalette palette;

    palette.setColor(QPalette::Window, kDarkBackground);
    palette.setColor(QPalette::WindowText, kDarkText);
    palette.setColor(QPalette::Base, kDarkBase);
    palette.setColor(QPalette::AlternateBase, kDarkBackground);
    palette.setColor(QPalette::ToolTipBase, kDarkText);
    palette.setColor(QPalette::ToolTipText, kDarkText);
    palette.setColor(QPalette::Text, kDarkText);
    palette.setColor(QPalette::Button, kDarkBackground);
    palette.setColor(QPalette::ButtonText, kDarkText);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, kHighlight);
    palette.setColor(QPalette::Highlight, kHighlight);
    palette.setColor(QPalette::HighlightedText, Qt::black);

    QApplication::setPalette(palette);

    // Fine-tune the scrollbars via stylesheet
    const QString styleSheet = R"(
        QScrollBar {
            background: #222;
            width: 16px;
        }
        QScrollBar::handle {
            background: #557;
            border-radius: 4px;
        }
        QScrollBar::handle:hover {
            background: #668;
        }

        QScrollBar::add-line, QScrollBar::sub-line {
            background: none;
        }
    )";
    qApp->setStyleSheet(styleSheet);
}
