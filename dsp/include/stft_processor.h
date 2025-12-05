#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

// Forward declarations
class IFFTProcessor;
class FFTWindow;
class SampleBuffer;

/**
 * @brief Short-Time Fourier Transform (STFT) processor
 *
 * Computes a time-frequency representation (spectrogram) by applying FFT
 * to overlapping windowed segments of audio samples. Orchestrates the
 * interaction between SampleBuffer (audio source), FFTWindow (windowing),
 * and IFFTProcessor (frequency analysis).
 *
 * The resulting spectrogram is a 2D matrix where:
 * - Outer dimension (rows): Time windows [0..window_count-1]
 * - Inner dimension (cols): Frequency bins [0..transform_size/2] (DC to Nyquist)
 *
 * Example usage:
 * @code
 *   FFTProcessor fft(512);
 *   FFTWindow window(512, FFTWindow::Type::kHann);
 *   SampleBuffer buffer(44100);
 *   buffer.AddSamples(audio_data);
 *
 *   STFTProcessor stft(fft, window, buffer);
 *   auto spectrogram = stft.ComputeSpectrogram(0, 256, 100);  // 50% overlap, 100 windows
 * @endcode
 */
class STFTProcessor
{
  public:
    /**
     * @brief Constructor
     * @param aFFTProcessor FFT processor for frequency analysis
     * @param aWindow Window function to apply to each segment
     * @param aBuffer Audio sample source
     * @throws std::invalid_argument if aWindow size != FFT transform size
     *
     * The window size must match the FFT transform size to ensure correct
     * frequency resolution. This constraint is validated at construction time.
     */
    STFTProcessor(IFFTProcessor& aFFTProcessor, FFTWindow& aWindow, const SampleBuffer& aBuffer);

    /**
     * @brief Compute spectrogram from audio samples
     * @param aFirstSample Starting sample index in the buffer (can be negative for zero-padding)
     * @param aWindowStride Hop size in samples between consecutive windows (must be > 0)
     * @param aWindowCount Number of time windows to compute
     * @return 2D vector [aWindowCount][transform_size/2 + 1] containing frequency magnitudes
     * @throws std::invalid_argument if aWindowStride is zero
     *
     * For each time window at position `aFirstSample + i * aWindowStride`:
     * 1. Extract samples from buffer (with zero-padding for out-of-bounds indices)
     * 2. Apply window function to extracted samples
     * 3. Compute FFT magnitudes from windowed samples
     * 4. Store result in output matrix
     *
     * Common overlap settings:
     * - No overlap: aWindowStride = window_size
     * - 50% overlap: aWindowStride = window_size / 2
     * - 75% overlap: aWindowStride = window_size / 4
     */
    [[nodiscard]] std::vector<std::vector<float>> ComputeSpectrogram(int64_t aFirstSample,
                                                                     size_t aWindowStride,
                                                                     size_t aWindowCount) const;

  private:
    IFFTProcessor& mFFTProcessor;
    FFTWindow& mWindow;
    const SampleBuffer& mBuffer;
};
