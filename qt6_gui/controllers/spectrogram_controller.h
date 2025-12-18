#pragma once
#include "models/audio_buffer.h"
#include "models/settings.h"
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
    static constexpr size_t KDefaultFftSize = 2048;
    static constexpr auto KDefaultWindowType = FFTWindow::Type::Hann;

    using FFTProcessorFactory = std::function<std::unique_ptr<IFFTProcessor>(size_t)>;
    using FFTWindowFactory = std::function<std::unique_ptr<FFTWindow>(size_t, FFTWindow::Type)>;

    /**
     * @brief Constructor
     * @param aSettings Reference to application settings model
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
    SpectrogramController(const Settings& aSettings,
                          const AudioBuffer& aAudioBuffer,
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
     * @brief Notify controller that FFT settings have changed
     *
     * Recreates FFTProcessor and FFTWindow for each channel.
     */
    void OnFFTSettingsChanged();

    /**
     * @brief Get reference to application settings
     * @return Reference to Settings instance
     */
    [[nodiscard]] const Settings& GetSettings() const { return mSettings; }

  private:
    const Settings& mSettings;       // Reference to application settings model
    const AudioBuffer& mAudioBuffer; // Reference to audio buffer model

    // FFT processing components (per channel)
    std::vector<std::unique_ptr<IFFTProcessor>> mFFTProcessors;
    std::vector<std::unique_ptr<FFTWindow>> mFFTWindows;

    FFTProcessorFactory mFFTProcessorFactory;
    FFTWindowFactory mFFTWindowFactory;

    // Spectrogram row cache.  Key: (channel, first sample).  Stores a single row
    // of spectrogram data for reuse.
    mutable std::map<std::pair<size_t, int64_t>, std::vector<float>> mSpectrogramRowCache;
};
