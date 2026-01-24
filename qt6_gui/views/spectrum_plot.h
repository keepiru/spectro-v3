// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include "audio_types.h"
#include <QLine>
#include <QObject>
#include <QRect>
#include <QString>
#include <QWidget>
#include <format>
#include <ostream>

class SpectrogramController;

/// @brief Real-time frequency spectrum line plot widget
///
/// Displays a line plot of the current frequency spectrum with frequency on the
/// horizontal axis and magnitude on the vertical axis.
///
/// Future features:
/// - Real-time spectrum line plot
/// - Frequency axis labels
/// - dB scale on vertical axis
/// - Peak markers
/// - Grid lines
class SpectrumPlot : public QWidget
{
    Q_OBJECT

  public:
    explicit SpectrumPlot(const SpectrogramController& aController, QWidget* parent = nullptr);
    ~SpectrumPlot() override = default;

  protected:
    void paintEvent(QPaintEvent* event) override;

    // The vertical scale paramaters used by GenerateDecibelScaleMarkers().
    struct DecibelScaleParameters
    {
        float aperture_min_decibels; // Passed through from settings
        float aperture_max_decibels; // Passed through from settings
        float pixels_per_decibel;    // May be negative for inverted scale
        int decibel_step;            // May be negative for inverted scale
        float top_marker_decibels;   // Decibel value of the topmost marker
        int marker_count;            // Number of markers to generate

        // Comparison and output operators for testing
        friend bool operator==(const DecibelScaleParameters& aLHS,
                               const DecibelScaleParameters& aRHS) = default;
        friend std::ostream& operator<<(std::ostream& aOS, const DecibelScaleParameters& aParams)
        {
            aOS << std::format(
              "DecibelScaleParameters(aperture_min_decibels={}, aperture_max_decibels={}, "
              "pixels_per_decibel={}, decibel_step={}, top_marker_decibels={}, marker_count={})",
              aParams.aperture_min_decibels,
              aParams.aperture_max_decibels,
              aParams.pixels_per_decibel,
              aParams.decibel_step,
              aParams.top_marker_decibels,
              aParams.marker_count);
            return aOS;
        }
    };

    // A single scale marker (tick mark line and label), for decibel scale or
    // crosshair
    struct Marker
    {
        QLine line;   // Line for the tick mark
        QRect rect;   // Rectangle for the label position
        QString text; // value for the marker

        friend bool operator==(const Marker& aLHS, const Marker& aRHS) = default;
        friend std::ostream& operator<<(std::ostream& aOS, const Marker& aMarker)
        {
            aOS << std::format("Marker(line=({}, {}, {}, {}),rect=({}, {}, {}, {}),'{}')",
                               aMarker.line.x1(),
                               aMarker.line.y1(),
                               aMarker.line.x2(),
                               aMarker.line.y2(),
                               aMarker.rect.x(),
                               aMarker.rect.y(),
                               aMarker.rect.width(),
                               aMarker.rect.height(),
                               aMarker.text.toStdString());
            return aOS;
        }
    };

    /// @brief Get the decibel values for the most recent stride in a given channel
    /// @param aChannel Channel index (0-based)
    /// @return std::vector<float> Vector of decibel values for the current FFT window
    /// @throws std::out_of_range if aChannel is out of range
    [[nodiscard]] std::vector<float> GetDecibels(ChannelCount aChannel) const;

    /// @brief Compute the points for plotting from decibel values
    /// @param aDecibels Vector of decibel values
    /// @param aWidth Width of the plot area in pixels
    /// @param aHeight Height of the plot area in pixels
    /// @return QPolygonF Polygon of points for plotting
    /// @note Points outside the given width are not included
    /// @note If the decibel range is zero, an empty polygon is returned
    [[nodiscard]] QPolygonF ComputePoints(const std::vector<float>& aDecibels,
                                          size_t aWidth,
                                          size_t aHeight) const;

    /// @brief Calculate decibel scale parameters for the plot
    /// @param aHeight Height of the plot area in pixels
    /// @return DecibelScaleParameters Decibel scale parameters
    ///
    /// These parameters cover the vertical spacing, range, count, step, etc for
    /// the decibel scale markers.  Horizontal positions are handled in
    /// GenerateDecibelScaleMarkers().
    [[nodiscard]] DecibelScaleParameters CalculateDecibelScaleParameters(int aHeight) const;

    /// @brief Generate decibel scale markers for the plot
    /// @param aParams Decibel scale parameters
    /// @param aWidth Width of the plot area in pixels
    /// @return std::vector<Marker> Vector of decibel markers
    [[nodiscard]] static std::vector<Marker> GenerateDecibelScaleMarkers(
      const DecibelScaleParameters& aParams,
      int aWidth);

    /// @brief Compute crosshair lines and labels for the given mouse position
    /// @param aMousePos Mouse position in widget coordinates
    /// @param aHeight Height of the plot area in pixels
    /// @param aWidth Width of the plot area in pixels
    /// @return std::array<Marker, 2> Array containing frequency and decibel markers
    [[nodiscard]] std::array<Marker, 2> ComputeCrosshair(QPoint aMousePos,
                                                         int aHeight,
                                                         int aWidth) const;

  private:
    const SpectrogramController& mController;
};
