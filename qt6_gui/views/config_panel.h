#ifndef CONFIG_PANEL_H
#define CONFIG_PANEL_H

#include <QWidget>

/**
 * @brief Configuration panel widget
 *
 * Provides controls for adjusting FFT parameters, display settings, and audio
 * device selection.
 *
 * Future controls:
 * - FFT size selector
 * - Window type selector
 * - Hop size/overlap controls
 * - Color map selector
 * - Audio device selector
 * - Sample rate display
 * - Display range controls (min/max dB)
 */
class ConfigPanel : public QWidget
{
    Q_OBJECT

  public:
    explicit ConfigPanel(QWidget* parent = nullptr);
    ~ConfigPanel() override = default;

  private:
    // Future: Control widgets (combo boxes, sliders, etc.)
};

#endif // CONFIG_PANEL_H
