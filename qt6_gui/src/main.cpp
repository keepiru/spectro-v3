#include <QApplication>
#include <QLabel>
#include <QMainWindow>

int
main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Spectro-v3 - Real-time Spectrum Analyzer");
    mainWindow.resize(1200, 800);

    auto* centralWidget =
      new QLabel("Spectro-v3\n\nSpectrum analyzer UI will be implemented here.");
    centralWidget->setAlignment(Qt::AlignCenter);
    mainWindow.setCentralWidget(centralWidget);

    mainWindow.show();

    return QApplication::exec();
}
