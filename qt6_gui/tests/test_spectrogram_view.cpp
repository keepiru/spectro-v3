#include "views/spectrogram_view.h"

#include <QTest>

class TestSpectrogramView : public QObject
{
    Q_OBJECT

  private slots:
    void testConstructor()
    {
        SpectrogramView view;
        QVERIFY(view.minimumWidth() > 0);
        QVERIFY(view.minimumHeight() > 0);
    }

    void testIsWidget()
    {
        SpectrogramView view;
        QVERIFY(qobject_cast<QWidget*>(&view) != nullptr);
    }
};

QTEST_MAIN(TestSpectrogramView)
#include "test_spectrogram_view.moc"
