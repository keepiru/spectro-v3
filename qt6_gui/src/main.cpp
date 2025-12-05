#include "controllers/main_window.h"
#include <QApplication>

int
main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    spectro_qt6::MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}