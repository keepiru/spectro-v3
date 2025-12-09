#include "main_window.h"

#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <Qt>

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
    // Placeholder widget - will be replaced with actual spectrum analyzer views
    auto placeholderLabel =
      new QLabel("Spectro-v3\n\nSpectrum analyzer UI will be implemented here.");
    placeholderLabel->setAlignment(Qt::AlignCenter);
    setCentralWidget(placeholderLabel);
}

void
MainWindow::setupConnections()
{
    // Future: Connect signals/slots between AudioBuffer, SpectrogramController,
    // and view widgets
}
