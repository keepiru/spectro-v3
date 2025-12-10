#include "views/config_panel.h"

#include <QTest>

class TestConfigPanel : public QObject
{
    Q_OBJECT

  private slots:
    void testConstructor()
    {
        const ConfigPanel panel;
        QVERIFY(panel.minimumWidth() > 0);
    }

    void testIsWidget()
    {
        ConfigPanel panel;
        QVERIFY(qobject_cast<QWidget*>(&panel) != nullptr);
    }
};

QTEST_MAIN(TestConfigPanel)
#include "test_config_panel.moc"
