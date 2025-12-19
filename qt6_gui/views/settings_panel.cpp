#include "settings_panel.h"
#include "models/settings.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <Qt>

SettingsPanel::SettingsPanel(Settings& aSettings, QWidget* parent)
  : QWidget(parent)
  , mSettings(&aSettings)
{
    constexpr int kPanelWidth = 300;
    setFixedWidth(kPanelWidth);

    auto* layout = new QVBoxLayout(this);
    auto* label = new QLabel("Configuration Panel\n\nControls will be added here.");
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    layout->addStretch();
}
