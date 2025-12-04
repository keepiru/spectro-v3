#include "main_window.h"
#include <QSignalSpy>
#include <QTest>

namespace spectro_qt6 {

class TestMainWindow : public QObject
{
    Q_OBJECT

  private slots:
    void testConstruction();
    void testWindowTitle();
    void testWindowSize();
};

void
TestMainWindow::testConstruction()
{
    // Test that MainWindow can be constructed without crashing
    MainWindow window;
    QVERIFY(true);
}

void
TestMainWindow::testWindowTitle()
{
    MainWindow window;
    QCOMPARE(window.windowTitle(), QString("Spectro"));
}

void
TestMainWindow::testWindowSize()
{
    MainWindow window;
    QCOMPARE(window.width(), 800);
    QCOMPARE(window.height(), 600);
}

} // namespace spectro_qt6

// QTEST_MAIN creates a QApplication for widget tests
QTEST_MAIN(spectro_qt6::TestMainWindow)
#include "test_main_window.moc"
