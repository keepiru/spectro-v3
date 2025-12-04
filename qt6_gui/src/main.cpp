#include "main_window.h"
#include <QApplication>

int
main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    spectro::MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}