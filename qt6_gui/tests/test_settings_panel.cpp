#include "views/settings_panel.h"

#include <QTest>

class TestSettingsPanel : public QObject
{
    Q_OBJECT

  private slots:
    static void TestConstructor()
    {
        const SettingsPanel panel;
        QVERIFY(panel.minimumWidth() > 0);
    }

    static void TestIsWidget()
    {
        SettingsPanel panel;
        QVERIFY(qobject_cast<QWidget*>(&panel) != nullptr);
    }
};

QTEST_MAIN(TestSettingsPanel)
#include "test_settings_panel.moc"
