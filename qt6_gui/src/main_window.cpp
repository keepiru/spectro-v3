#include "main_window.h"

#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "views/config_panel.h"
#include "views/spectrogram_view.h"
#include "views/spectrum_plot.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <fft_processor.h>
#include <fft_window.h>

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
{
    constexpr int kDefaultWindowWidth = 1200;
    constexpr int kDefaultWindowHeight = 800;
    constexpr size_t kDefaultChannelCount = 2;
    constexpr size_t kDefaultSampleRate = 44100;
    constexpr size_t kDefaultStride = 256;

    setWindowTitle("Spectro-v3 - Real-time Spectrum Analyzer");
    resize(kDefaultWindowWidth, kDefaultWindowHeight);

    // Create models and controllers
    mAudioBuffer = new AudioBuffer(kDefaultChannelCount, kDefaultSampleRate, this);

    // for testing, load some frequency sweeps into the audio buffer
    {
        const size_t durationSeconds = 10;
        const size_t totalSamples = durationSeconds * kDefaultSampleRate;
        std::vector<float> samples;
        samples.reserve(totalSamples * kDefaultChannelCount);

        for (size_t i = 0; i < totalSamples; i++) {
            float t = static_cast<float>(i) / static_cast<float>(kDefaultSampleRate);
            // Frequency sweep from 20 Hz to 20 kHz over duration
            float freq = 20.0f * std::pow(1000.0f, t / static_cast<float>(durationSeconds));
            float sampleValue = 0.1f * std::sin(2.0f * static_cast<float>(M_PI) * freq * t);

            // Stereo: same signal on both channels
            samples.push_back(sampleValue); // Left channel
            samples.push_back(sampleValue); // Right channel
        }

        mAudioBuffer->AddSamples(samples);
    }

    mSpectrogramController = new SpectrogramController(*mAudioBuffer, nullptr, nullptr, this);

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
    leftLayout->addWidget(mSpectrogramView, 7); // 70% of vertical space
    leftLayout->addWidget(mSpectrumPlot, 3);    // 30% of vertical space

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
}
