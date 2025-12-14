#include "config_panel.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <Qt>

ConfigPanel::ConfigPanel(QWidget* parent)
  : QWidget(parent)
{
    constexpr int kPanelWidth = 300;
    setFixedWidth(kPanelWidth);

    auto* layout = new QVBoxLayout(this);
    auto* label = new QLabel("Configuration Panel\n\nControls will be added here.");
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    layout->addStretch();
}
