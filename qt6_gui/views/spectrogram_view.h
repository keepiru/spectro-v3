#pragma once

#include "models/settings.h"
#include <QAbstractScrollArea>
#include <QScrollBar>
#include <QWidget>
#include <audio_types.h>
#include <functional>

// Forward declarations
class SpectrogramController;
class TestableSpectrogramView;

/// @brief Configuration data for rendering spectrogram
struct RenderConfig
{
    ChannelCount channels{};
    FFTSize stride;
    FramePosition top_frame;
    float min_decibels{};
    float max_decibels{};
    float decibel_range{};
    float inverse_decibel_range{};
    const Settings::ColorMapLUTs& color_map_lut;

    /// @brief Debugging/testing helper for inspecting RenderConfig instances.
    ///
    /// Converts the RenderConfig into a human-readable string, primarily for
    /// use in logging, assertions, and test failure messages where detailed
    /// comparison of render parameters is needed.
    friend std::string ToString(const RenderConfig& config)
    {
        return std::format(
          "RenderConfig{{\n channels={}\n stride={}\n top_frame={}\n "
          "min_decibels={}\n max_decibels={}\n decibel_range={}\n inverse_decibel_range={}\n "
          "color_map_lut_ref=<ptr:{}>}}",
          config.channels,
          config.stride.Get(),
          config.top_frame.Get(),
          config.min_decibels,
          config.max_decibels,
          config.decibel_range,
          config.inverse_decibel_range,
          static_cast<const void*>(&config.color_map_lut));
    }
};

/// @brief Waterfall spectrogram display widget
///
/// Displays a scrolling waterfall plot of the spectrogram with frequency on the
/// horizontal axis and time on the vertical axis. Colors represent magnitude.
///
/// Future features:
/// - Scrolling/scrubbing through time
/// - Configurable color maps (viridis, plasma, grayscale)
/// - Frequency and time axis labels
/// - dB scale display
/// - Zoom/pan controls
class SpectrogramView : public QAbstractScrollArea
{
    Q_OBJECT

  public:
    /// @brief Function type for triggering viewport updates
    ///
    /// Used to request viewport repaints when data or scroll position changes.
    /// This wraps QAbstractScrollArea's viewport()->update() in production,
    /// but can be overridden in tests to count update calls or simulate
    /// different behaviors.
    using ViewportUpdater = std::function<void()>;

    /// @brief Function type for getting a single viewport dimension in pixels
    ///
    /// Used to query the current viewport size (width or height) when
    /// calculating render parameters, scroll ranges, or layout.  These wrap
    /// QAbstractScrollArea's viewport accessors in prod, but can be overridden
    /// in tests to simulate different viewport sizes.
    using ViewportDimensionGetter = std::function<int()>;

    /// @brief Constructor
    /// @param aController Reference to spectrogram controller
    /// @param parent Qt parent widget (optional)
    explicit SpectrogramView(const SpectrogramController& aController, QWidget* parent = nullptr);
    ~SpectrogramView() override = default;

    /// @brief Generate spectrogram image for given dimensions
    /// @param aWidth Width in pixels
    /// @param aHeight Height in pixels
    /// @return Generated spectrogram image
    QImage GenerateSpectrogramImage(int aWidth, int aHeight);

    /// @brief Gather configuration needed for rendering
    /// @param aHeight Height in pixels (needed for topFrame calculation)
    /// @return RenderConfig struct with all settings and precomputed values
    [[nodiscard]] RenderConfig GetRenderConfig(size_t aHeight) const;

    /// @brief Update scrollbar range based on available audio data
    ///
    /// Called when new audio data arrives. Updates the scrollbar's maximum to
    /// reflect the total available frames. If the scrollbar is currently at its
    /// maximum (live mode), it will be updated to the new maximum to continue
    /// following live audio. Otherwise, the scroll position is preserved to
    /// maintain the user's historical viewing position.  Repaints the view if
    /// needed.
    ///
    /// @param aAvailableFrames Total number of frames available in the buffer
    void UpdateScrollbarRange(FrameCount aAvailableFrames);

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    const SpectrogramController& mController;
    FrameCount mPreviousAvailableFrames{ 0 };

    // These lambdas access the member functions of the QAbstractScrollArea's
    // viewport.  The defaults are used in production, but can be overridden in
    // tests via derived test fixture classes.
    ViewportUpdater mUpdateViewport;
    ViewportDimensionGetter mGetViewportWidth;
    ViewportDimensionGetter mGetViewportHeight;
    friend class TestableSpectrogramView;
};
