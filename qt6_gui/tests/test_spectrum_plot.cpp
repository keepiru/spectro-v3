#include "views/spectrum_plot.h"

#include <QTest>

class TestSpectrumPlot : public QObject
{
    Q_OBJECT

  private slots:
    void testConstructor()
    {
        SpectrumPlot plot;
        QVERIFY(plot.minimumWidth() > 0);
        QVERIFY(plot.minimumHeight() > 0);
    }

    void testIsWidget()
    {
        SpectrumPlot plot;
        QVERIFY(qobject_cast<QWidget*>(&plot) != nullptr);
    }
};

QTEST_MAIN(TestSpectrumPlot)
#include "test_spectrum_plot.moc"
