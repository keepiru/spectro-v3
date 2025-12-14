#include "main_window.h"
#include <QApplication>

int
main(int argc, char* argv[])
{
    QApplication const app(argc, argv);

    MainWindow mainWindow;
    mainWindow.show();

    return QApplication::exec();
}
