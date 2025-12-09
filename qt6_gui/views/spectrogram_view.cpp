#include "spectrogram_view.h"

#include <QPaintEvent>
#include <QPainter>

SpectrogramView::SpectrogramView(QWidget* parent)
  : QWidget(parent)
{
    setMinimumSize(400, 300);
}

void
SpectrogramView::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    // Draw placeholder text
    painter.setPen(Qt::white);
    painter.drawText(rect(), Qt::AlignCenter, "Spectrogram View\n(Waterfall)");
}
