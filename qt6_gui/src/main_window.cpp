#include "main_window.h"

#include "views/config_panel.h"
#include "views/spectrogram_view.h"
#include "views/spectrum_plot.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
{
    constexpr int kDefaultWindowWidth = 1200;
    constexpr int kDefaultWindowHeight = 800;

    setWindowTitle("Spectro-v3 - Real-time Spectrum Analyzer");
    resize(kDefaultWindowWidth, kDefaultWindowHeight);

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
    mSpectrogramView = new SpectrogramView(this);
    mSpectrumPlot = new SpectrumPlot(this);
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
