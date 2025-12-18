#pragma once

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
class SettingsPanel : public QWidget
{
    Q_OBJECT

  public:
    explicit SettingsPanel(QWidget* parent = nullptr);
    ~SettingsPanel() override = default;

  private:
    // Future: Control widgets (combo boxes, sliders, etc.)
};
