#include "models/settings.h"
#include "views/settings_panel.h"

#include <QTest>

class TestSettingsPanel : public QObject
{
    Q_OBJECT

  private slots:
    static void TestConstructor()
    {
        Settings settings;
        const SettingsPanel panel(settings);
        QVERIFY(panel.width() == 300);
    }
};

QTEST_MAIN(TestSettingsPanel)
#include "test_settings_panel.moc"
