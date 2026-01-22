// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

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
