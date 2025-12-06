#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <Qt>

int
main(int argc, char* argv[])
{
    QApplication const app(argc, argv);

    constexpr int kDefaultWindowWidth = 1200;
    constexpr int kDefaultWindowHeight = 800;

    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Spectro-v3 - Real-time Spectrum Analyzer");
    mainWindow.resize(kDefaultWindowWidth, kDefaultWindowHeight);

    auto* centralWidget =
      new QLabel("Spectro-v3\n\nSpectrum analyzer UI will be implemented here.");
    centralWidget->setAlignment(Qt::AlignCenter);
    mainWindow.setCentralWidget(centralWidget);

    mainWindow.show();

    return QApplication::exec();
}
