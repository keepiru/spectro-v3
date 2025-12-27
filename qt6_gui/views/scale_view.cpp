#include "scale_view.h"
#include "controllers/spectrogram_controller.h"
#include <QPaintEvent>
#include <QPainter>
#include <QWidget>

ScaleView::ScaleView(const SpectrogramController& aController, QWidget* parent)
  : QWidget(parent)
  , mController(aController)
{
    constexpr int kScaleHeight = 20;
    setFixedHeight(kScaleHeight);
}

void
ScaleView::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);

    // Fill with magenta
    painter.fillRect(rect(), Qt::magenta);
}
