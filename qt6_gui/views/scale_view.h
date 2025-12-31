#pragma once

#include "include/global_constants.h"
#include <QWidget>
#include <format>
#include <optional>
#include <ostream>

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
    struct TickMark
    {
        size_t position{};           // Horizontal in pixels
        std::optional<size_t> label; // Optional frequency label in Hz

        // Comparison operator for testing
        friend bool operator==(const TickMark& lhs, const TickMark& rhs) = default;

        // Stream output operator, used to aid in test failure diagnostics
        friend std::ostream& operator<<(std::ostream& aOS, const TickMark& aTick)
        {
            aOS << std::format("TickMark{{position: {}, label: {}}}",
                               aTick.position,
                               aTick.label.has_value() ? std::to_string(aTick.label.value())
                                                       : "none");
            return aOS;
        }
    };

    /**
     * @brief Constructor for ScaleView
     * @param aController Reference to SpectrogramController for accessing audio and settings data
     * @param parent Optional parent widget
     */
    explicit ScaleView(const SpectrogramController& aController, QWidget* parent = nullptr);
    ~ScaleView() override = default;

    /**
     * @brief Calculate tick marks for the frequency scale based on the current FFT settings
     * @param aWidth Width of the ScaleView in pixels
     * @return Vector of TickMark structures representing positions and labels
     */
    [[nodiscard]] std::vector<TickMark> CalculateTickMarks(size_t aWidth) const;

  protected:
    /**
     * @brief Paint event handler for rendering the frequency scale
     * @param event Paint event details
     */
    void paintEvent(QPaintEvent* event) override;

  private:
    const SpectrogramController& mController;
};
