#include "spectrogram_view.h"

#include "controllers/spectrogram_controller.h"

#include <QPaintEvent>
#include <QPainter>
#include <cmath>
#include <stdexcept>

SpectrogramView::SpectrogramView(SpectrogramController* aController, QWidget* parent)
  : QWidget(parent)
  , mController(aController)
{
    if (!aController) {
        throw std::invalid_argument("SpectrogramView: aController must not be null");
    }
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
