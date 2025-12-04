#include "main_window.h"

namespace spectro {
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

} // namespace spectro