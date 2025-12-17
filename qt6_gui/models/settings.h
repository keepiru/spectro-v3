#pragma once

#include <QObject>
#include <cstddef>
#include <fft_window.h>

// Forward declarations
class SpectrogramView;

/**
 * @brief Store runtime settings for the application.
 *
 * Owns all configurable application settings. Emits signals when settings
 * change. Components initialize from Settings and listen for changes.
 */
class Settings : public QObject
{
    Q_OBJECT

  public:
    explicit Settings(QObject* aParent = nullptr)
      : QObject(aParent)
    {
    }

    // FFT Settings
    [[nodiscard]] size_t GetFFTSize() const { return mFFTSize; }
    [[nodiscard]] FFTWindow::Type GetWindowType() const { return mWindowType; }
    [[nodiscard]] size_t GetWindowStride() const { return mWindowStride; }
    [[nodiscard]] float GetApertureMinDecibels() const { return mApertureMinDecibels; }
    [[nodiscard]] float GetApertureMaxDecibels() const { return mApertureMaxDecibels; }

    /**
     * @brief Set FFT settings
     * @param aTransformSize New FFT size (must be power of 2)
     * @param aWindowType New window function type
     * @throws std::invalid_argument if aTransformSize is zero
     *
     * These are set together because changing either requires recreating
     * FFT and window objects.
     */
    void SetFFTSettings(const size_t aTransformSize, const FFTWindow::Type aWindowType);
    void SetWindowStride(size_t aStride);

  signals:
    /**
     * @brief Emitted when FFT size or window type changes
     *
     * Listeners (SpectrogramController) should recreate FFT/window objects.
     */
    void FFTSettingsChanged();

    /**
     * @brief Emitted when window stride changes
     *
     * Listeners (SpectrogramController) should update stride and snap view position.
     */
    void WindowStrideChanged();

  private:
    size_t mFFTSize = 2048;
    FFTWindow::Type mWindowType = FFTWindow::Type::kHann;
    size_t mWindowStride = 1024;

    // The aperture is the visible decibel range in the spectrogram and spectrum
    // plot.
    float mApertureMinDecibels = -30.0f;
    float mApertureMaxDecibels = 30.0f;
};
