#include "config_panel.h"

#include <QLabel>
#include <QVBoxLayout>

ConfigPanel::ConfigPanel(QWidget* parent)
  : QWidget(parent)
{
    setFixedWidth(300);

    auto* layout = new QVBoxLayout(this);
    auto* label = new QLabel("Configuration Panel\n\nControls will be added here.");
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    layout->addStretch();
}
