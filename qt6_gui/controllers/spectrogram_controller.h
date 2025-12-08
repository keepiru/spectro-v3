#pragma once
#include <QObject>
#include <cstddef>
#include <cstdint>
#include <fft_window.h>
#include <functional>
#include <ifft_processor.h>
#include <memory>
#include <stft_processor.h>
#include <vector>

// Forward declarations
class AudioBuffer;

/**
 * @brief Controller for spectrogram data flow and view state
 *
 * Coordinates the data flow between AudioBuffer (model) and SpectrogramView (view).
 * Owns FFT processing components (STFTProcessor, FFTProcessor, FFTWindow) per channel.
 * Manages view state including live/historical mode and scroll position.
 */
class SpectrogramController : public QObject
{
    Q_OBJECT

  public:
    using FFTProcessorFactory = std::function<std::unique_ptr<IFFTProcessor>(size_t)>;
    using FFTWindowFactory = std::function<std::unique_ptr<FFTWindow>(size_t, FFTWindow::Type)>;

    /**
     * @brief Constructor
     * @param aAudioBuffer Reference to the audio buffer model
     * @param aFFTProcessorFactory Factory function to create FFTProcessor instances
     * @param aFFTWindowFactory Factory function to create FFTWindow instances
     * @param aParent Qt parent object (optional)
     *
     */
    SpectrogramController(AudioBuffer& aAudioBuffer,
                          FFTProcessorFactory aFFTProcessorFactory,
                          FFTWindowFactory aFFTWindowFactory,
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
     * Calls STFTProcessor.ComputeSpectrogram() to compute frequency data on-demand.
     * Each row represents one time window in the spectrogram.
     */
    [[nodiscard]] std::vector<std::vector<float>> GetRows(size_t aChannel,
                                                          int64_t aFirstSample,
                                                          size_t aRowCount) const;

  public slots:
    /**
     * @brief Set FFT settings (transform size, window function)
     * @param aTransformSize New FFT transform size (must be power of 2)
     * @param aWindowType New window function type
     * @throws std::invalid_argument if aTransformSize is zero
     *
     * Recreates FFTProcessor, FFTWindow, and STFTProcessor for each channel.
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
    std::vector<std::unique_ptr<STFTProcessor>> mSTFTProcessors;

    // FFT settings
    size_t mWindowStride;

    FFTProcessorFactory mFFTProcessorFactory;
    FFTWindowFactory mFFTWindowFactory;
};
