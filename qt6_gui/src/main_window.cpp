#include "main_window.h"
#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "models/audio_recorder.h"
#include "models/settings.h"
#include "views/config_panel.h"
#include "views/spectrogram_view.h"
#include "views/spectrum_plot.h"
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMediaDevices>
#include <QOverload>
#include <QVBoxLayout>
#include <QWidget>
#include <cmath>
#include <cstddef>

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , mAudioRecorder(new AudioRecorder(this))
{
    constexpr int kDefaultWindowWidth = 1400;
    constexpr int kDefaultWindowHeight = 800;
    constexpr size_t kDefaultChannelCount = 2;
    constexpr size_t kDefaultSampleRate = 44100;
    constexpr size_t kDefaultStride = 1024;

    setWindowTitle("Spectro-v3 - Real-time Spectrum Analyzer");
    resize(kDefaultWindowWidth, kDefaultWindowHeight);

    // Create models and controllers
    mAudioBuffer = new AudioBuffer(kDefaultChannelCount, kDefaultSampleRate, this);

    // For now we don't have a config UI, so just start recording with defaults
    mAudioRecorder->Start(mAudioBuffer, QMediaDevices::defaultAudioInput());

    mSettings = new Settings(this);
    mSpectrogramController =
      new SpectrogramController(*mSettings, *mAudioBuffer, nullptr, nullptr, this);

    // Set initial stride
    mSpectrogramController->SetWindowStride(kDefaultStride);

    createLayout();
    setupConnections();
}

void
MainWindow::createLayout()
{
    /**
     * ┌─────────────────────────────────────────────────────────┐
     * │ QHBoxLayout (main)                                      │
     * │ ┌──────────────────────────────┬────────────────────┐   │
     * │ │ QVBoxLayout (left)           │ ConfigPanel        │   │
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
    mConfigPanel = new ConfigPanel(this);

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
    mainLayout->addWidget(leftContainer, 1); // Takes remaining space
    mainLayout->addWidget(mConfigPanel, 0);  // Fixed width (no stretch)

    setCentralWidget(mainWidget);
}

void
MainWindow::setupConnections()
{
    // Future: Connect signals/slots between AudioBuffer, SpectrogramController,
    // and view widgets

    // Update SpectrogramView when new audio data is available
    connect(mAudioBuffer,
            &AudioBuffer::dataAvailable,
            mSpectrogramView,
            QOverload<>::of(&SpectrogramView::update));

    // Also update SpectrumPlot
    connect(mAudioBuffer,
            &AudioBuffer::dataAvailable,
            mSpectrumPlot,
            QOverload<>::of(&SpectrumPlot::update));
}
