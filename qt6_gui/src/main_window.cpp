#include "main_window.h"
#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "models/audio_recorder.h"
#include "models/settings.h"
#include "views/settings_panel.h"
#include "views/spectrogram_view.h"
#include "views/spectrum_plot.h"
#include <QHBoxLayout>
#include <QLoggingCategory>
#include <QMainWindow>
#include <QMediaDevices>
#include <QOverload>
#include <QVBoxLayout>
#include <QWidget>
#include <cmath>
#include <cstddef>

namespace {
constexpr size_t KDefaultChannelCount = 2;
constexpr size_t KDefaultSampleRate = 44100;
}

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , mAudioBuffer(new AudioBuffer(KDefaultChannelCount, KDefaultSampleRate, this))
  , mAudioRecorder(new AudioRecorder(*mAudioBuffer, this))
  , mSettings(new Settings(this))
{
    constexpr int kDefaultWindowWidth = 1400;
    constexpr int kDefaultWindowHeight = 800;
    constexpr size_t kDefaultStride = 1024;

    setWindowTitle("Spectro-v3 - Real-time Spectrum Analyzer");
    resize(kDefaultWindowWidth, kDefaultWindowHeight);

    // Suppress Qt multimedia FFmpeg logging noise
    QLoggingCategory::setFilterRules("qt.multimedia.ffmpeg=false");

    // For now we don't have a config UI, so just start recording with defaults
    mAudioRecorder->Start(
      QMediaDevices::defaultAudioInput(), KDefaultChannelCount, KDefaultSampleRate);

    mSpectrogramController =
      new SpectrogramController(*mSettings, *mAudioBuffer, nullptr, nullptr, this);

    CreateLayout();
    SetupConnections();
}

void
MainWindow::CreateLayout()
{
    /**
     * ┌─────────────────────────────────────────────────────────┐
     * │ QHBoxLayout (main)                                      │
     * │ ┌──────────────────────────────┬────────────────────┐   │
     * │ │ QVBoxLayout (left)           │ SettingsPanel      │   │
     * │ │ ┌──────────────────────────┐ │ (~300px)           │   │
     * │ │ │ SpectrogramView          │ │                    │   │
     * │ │ │ (stretch 7)              │ │                    │   │
     * │ │ └──────────────────────────┘ │                    │   │
     * │ │ ┌──────────────────────────┐ │                    │   │
     * │ │ │ SpectrumPlot             │ │                    │   │
     * │ │ │ (stretch 3)              │ │                    │   │
     * │ │ └──────────────────────────┘ │                    │   │
     * │ └──────────────────────────────┴────────────────────┘   │
     * └─────────────────────────────────────────────────────────┘
     */

    // Create view widgets
    mSpectrogramView = new SpectrogramView(mSpectrogramController, this);
    mSpectrumPlot = new SpectrumPlot(mSpectrogramController, this);
    mSettingsPanel = new SettingsPanel(*mSettings, this);

    // Create left container with vertical layout for the two plots
    auto* leftContainer = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    constexpr int kSpectrogramStretch = 7; // 70% of vertical space
    constexpr int kSpectrumStretch = 3;    // 30% of vertical space
    leftLayout->addWidget(mSpectrogramView, kSpectrogramStretch);
    leftLayout->addWidget(mSpectrumPlot, kSpectrumStretch);

    // Create main horizontal layout
    auto* mainWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(mainWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(leftContainer, 1);  // Takes remaining space
    mainLayout->addWidget(mSettingsPanel, 0); // Fixed width (no stretch)

    setCentralWidget(mainWidget);
}

void
MainWindow::SetupConnections()
{
    // Future: Connect signals/slots between AudioBuffer, SpectrogramController,
    // and view widgets

    // Update SpectrogramView when new audio data is available
    connect(mAudioBuffer,
            &AudioBuffer::DataAvailable,
            mSpectrogramView,
            QOverload<>::of(&SpectrogramView::update));

    // Also update SpectrumPlot
    connect(mAudioBuffer,
            &AudioBuffer::DataAvailable,
            mSpectrumPlot,
            QOverload<>::of(&SpectrumPlot::update));
}
