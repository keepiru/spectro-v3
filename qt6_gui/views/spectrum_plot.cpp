#include "spectrum_plot.h"

#include <QPaintEvent>
#include <QPainter>

SpectrumPlot::SpectrumPlot(QWidget* parent)
  : QWidget(parent)
{
    setMinimumSize(400, 200);
}

void
SpectrumPlot::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    // Draw placeholder text
    painter.setPen(Qt::white);
    painter.drawText(rect(), Qt::AlignCenter, "Spectrum Plot\n(Line Plot)");
}
