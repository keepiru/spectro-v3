#pragma once

#include <QWidget>

class SpectrogramController;

/**
 * @brief Scale widget for frequency axis display
 *
 * Displays a frequency scale between the spectrogram and spectrum plot.
 * This widget has a fixed height and shows frequency markers.
 */
class ScaleView : public QWidget
{
    Q_OBJECT

  public:
    /**
     * @brief Constructor for ScaleView
     * @param aController Reference to SpectrogramController for accessing audio and settings data
     * @param parent Optional parent widget
     */
    explicit ScaleView(const SpectrogramController& aController, QWidget* parent = nullptr);
    ~ScaleView() override = default;

  protected:
    /**
     * @brief Paint event handler for rendering the frequency scale
     * @param event Paint event details
     */
    void paintEvent(QPaintEvent* event) override;

  private:
    const SpectrogramController& mController;
};
