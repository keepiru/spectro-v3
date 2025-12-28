#pragma once

#include <QLine>
#include <QObject>
#include <QRect>
#include <QString>
#include <QWidget>
#include <format>
#include <ostream>

class SpectrogramController;

/**
 * @brief Real-time frequency spectrum line plot widget
 *
 * Displays a line plot of the current frequency spectrum with frequency on the
 * horizontal axis and magnitude on the vertical axis.
 *
 * Future features:
 * - Real-time spectrum line plot
 * - Frequency axis labels
 * - dB scale on vertical axis
 * - Peak markers
 * - Grid lines
 */
class SpectrumPlot : public QWidget
{
    Q_OBJECT

  public:
    struct DecibelMarker
    {
        QLine line;   // Line for the tick mark
        QRect rect;   // Rectangle for the label position
        QString text; // Decibel value for the marker

        friend bool operator==(const DecibelMarker& lhs, const DecibelMarker& rhs) = default;
        friend std::ostream& operator<<(std::ostream& ostream, const DecibelMarker& marker)
        {
            ostream << std::format(
              "DecibelMarker(line=({}, {}, {}, {}),rect=({}, {}, {}, {}),'{}')",
              marker.line.x1(),
              marker.line.y1(),
              marker.line.x2(),
              marker.line.y2(),
              marker.rect.x(),
              marker.rect.y(),
              marker.rect.width(),
              marker.rect.height(),
              marker.text.toStdString());
            return ostream;
        }
    };

    explicit SpectrumPlot(const SpectrogramController& aController, QWidget* parent = nullptr);
    ~SpectrumPlot() override = default;

    /**
     * @brief Get the decibel values for the most recent stride in a given channel
     * @param aChannel Channel index (0-based)
     * @return std::vector<float> Vector of decibel values for the current FFT window
     * @throws std::out_of_range if aChannel is out of range
     */
    [[nodiscard]] std::vector<float> GetDecibels(size_t aChannel) const;

    /**
     * @brief Compute the points for plotting from decibel values
     * @param aDecibels Vector of decibel values
     * @param aWidth Width of the plot area in pixels
     * @param aHeight Height of the plot area in pixels
     * @return QPolygonF Polygon of points for plotting
     * @note Points outside the given width are not included
     * @note If the decibel range is zero, an empty polygon is returned
     */
    [[nodiscard]] QPolygonF ComputePoints(const std::vector<float>& aDecibels,
                                          size_t aWidth,
                                          size_t aHeight) const;

    [[nodiscard]] std::vector<DecibelMarker> GenerateDecibelScaleMarkers(int aWidth,
                                                                         int aHeight) const;

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    const SpectrogramController& mController;
};
