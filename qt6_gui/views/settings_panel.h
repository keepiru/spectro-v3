#pragma once

#include "models/settings.h"
#include <QComboBox>
#include <QSlider>
#include <QWidget>
#include <array>

// Forward declarations
class QLabel;

/**
 * @brief Configuration panel widget
 *
 * Provides controls for adjusting FFT parameters, display settings, and color maps.
 */
class SettingsPanel : public QWidget
{
    Q_OBJECT

  public:
    explicit SettingsPanel(Settings& aSettings, QWidget* parent = nullptr);
    ~SettingsPanel() override = default;

  private:
    Settings* mSettings = nullptr;

    // Control widgets
    QComboBox* mWindowTypeCombo = nullptr;
    QComboBox* mFFTSizeCombo = nullptr;
    QSlider* mWindowScaleSlider = nullptr;
    QLabel* mWindowScaleLabel = nullptr;
    QSlider* mApertureMinSlider = nullptr;
    QLabel* mApertureMinLabel = nullptr;
    QSlider* mApertureMaxSlider = nullptr;
    QLabel* mApertureMaxLabel = nullptr;

    static constexpr size_t KNumColorMapSelectors = 6;
    std::array<QComboBox*, KNumColorMapSelectors> mColorMapCombos = {};

    // Helper methods
    void CreateLayout();
    void CreateWindowTypeControl(class QFormLayout* aLayout);
    void CreateFFTSizeControl(class QFormLayout* aLayout);
    void CreateWindowScaleControl(class QFormLayout* aLayout);
    void CreateApertureControls(class QFormLayout* aLayout);
    void CreateColorMapControls(class QFormLayout* aLayout);
    void UpdateWindowScaleLabel();
    void UpdateApertureMinLabel();
    void UpdateApertureMaxLabel();
};
