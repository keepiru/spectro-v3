#include "main_window.h"

namespace spectro_qt6 {
MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
{
    setupUi();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    setWindowTitle("Spectro");
    resize(800, 600);
}

} // namespace spectro_qt6