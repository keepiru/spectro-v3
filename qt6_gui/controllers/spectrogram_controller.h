#pragma once
#include "models/audio_buffer.h"
#include <QObject>
#include <cstddef>
#include <cstdint>
#include <fft_window.h>
#include <functional>
#include <ifft_processor.h>
#include <memory>
#include <vector>

/**
 * @brief Controller for spectrogram data flow and view state
 *
 * Coordinates the data flow between AudioBuffer (model) and SpectrogramView (view).
 * Owns FFT processing components (FFTProcessor, FFTWindow) per channel.
 * Manages view state including live/historical mode and scroll position.
 */
class SpectrogramController : public QObject
{
    Q_OBJECT

  public:
    static constexpr size_t kDefaultFFTSize = 2048;
    static constexpr auto kDefaultWindowType = FFTWindow::Type::kHann;

    using FFTProcessorFactory = std::function<std::unique_ptr<IFFTProcessor>(size_t)>;
    using FFTWindowFactory = std::function<std::unique_ptr<FFTWindow>(size_t, FFTWindow::Type)>;

    /**
     * @brief Constructor
     * @param aAudioBuffer Reference to the audio buffer model
     * @param aFFTProcessorFactory Factory function to create FFTProcessor instances (optional)
     * @param aFFTWindowFactory Factory function to create FFTWindow instances (optional)
     * @param aParent Qt parent object (optional)
     *
     * The factories are used for dependency injection in tests.  By default,
     * FFTProcessor and FFTWindow instances are created, which is appropriate
     * for production use.
     *
     */
    SpectrogramController(AudioBuffer& aAudioBuffer,
                          FFTProcessorFactory aFFTProcessorFactory = nullptr,
                          FFTWindowFactory aFFTWindowFactory = nullptr,
                          QObject* aParent = nullptr);

    /**
     * @brief Destructor
     */
    ~SpectrogramController() override = default;

    // Rule of five
    SpectrogramController(const SpectrogramController&) = delete;
    SpectrogramController& operator=(const SpectrogramController&) = delete;
    SpectrogramController(SpectrogramController&&) = delete;
    SpectrogramController& operator=(SpectrogramController&&) = delete;

    /**
     * @brief Get spectrogram rows for a channel
     * @param aChannel Channel index (0-based)
     * @param aFirstSample First sample position (aligned to stride)
     * @param aRowCount Number of rows to compute
     * @return 2D vector [aRowCount][frequency_bins] containing frequency magnitudes
     * @throws std::out_of_range if aChannel is invalid
     *
     * Each row represents one time window in the spectrogram.
     */
    [[nodiscard]] std::vector<std::vector<float>> GetRows(size_t aChannel,
                                                          int64_t aFirstSample,
                                                          size_t aRowCount) const;

    /**
     * @brief Get the number of available samples
     * @return Number of samples currently available in the audio buffer
     */
    [[nodiscard]] size_t GetAvailableSampleCount() const;

    /**
     * @brief Get the number of available channels
     * @return Number of audio channels
     */
    [[nodiscard]] size_t GetChannelCount() const;

    /**
     * @brief Get aperture minimum decibel setting
     * @return Minimum decibel value for aperture mapping
     */
    [[nodiscard]] float GetApertureMinDecibels() const { return mApertureMinDecibels; }

    /**
     * @brief Get aperture maximum decibel setting
     * @return Maximum decibel value for aperture mapping
     */
    [[nodiscard]] float GetApertureMaxDecibels() const { return mApertureMaxDecibels; }
  public slots:
    /**
     * @brief Set FFT settings (transform size, window function)
     * @param aTransformSize New FFT transform size (must be power of 2)
     * @param aWindowType New window function type
     * @throws std::invalid_argument if aTransformSize is zero
     *
     * Recreates FFTProcessor and FFTWindow for each channel.
     */
    void SetFFTSettings(const size_t aTransformSize, const FFTWindow::Type aWindowType);

    /**
     * @brief Set window stride
     * @param aStride New window stride in samples (must be > 0)
     * @throws std::invalid_argument if aStride is zero
     */
    void SetWindowStride(const size_t aStride);

    /**
     * @brief Get current window stride
     * @return Current window stride in samples
     */
    [[nodiscard]] size_t GetWindowStride() const { return mWindowStride; }

  private:
    AudioBuffer& mAudioBuffer; // Reference to audio buffer model

    // FFT processing components (per channel)
    std::vector<std::unique_ptr<IFFTProcessor>> mFFTProcessors;
    std::vector<std::unique_ptr<FFTWindow>> mFFTWindows;

    // FFT settings
    size_t mWindowStride;

    FFTProcessorFactory mFFTProcessorFactory;
    FFTWindowFactory mFFTWindowFactory;

    // The aperture is the visible decibel range in the spectrogram and spectrum
    // plot.
    float mApertureMinDecibels = -30.0f;
    float mApertureMaxDecibels = 30.0f;

    // Spectrogram row cache.  Key: (channel, first sample).  Stores a single row
    // of spectrogram data for reuse.
    mutable std::map<std::pair<size_t, int64_t>, std::vector<float>> mSpectrogramRowCache;
};
