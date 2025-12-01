#pragma once
#include <cstdint>
#include <span>
#include <vector>

// Forward declaration for FFTW complex type
using fftwf_complex = float[2];

/**
 * @brief Interface for FFT processors
 *
 * Pure virtual interface for computing FFT operations on audio samples.
 * Enables dependency injection and mock implementations for testing.
 */
class IFFTProcessor
{
  public:
    virtual ~IFFTProcessor() = default;

    /**
     * @brief Get the number of frequency bins
     * @return Number of frequency bins configured for this processor
     */
    virtual uint32_t getNumBins() const noexcept = 0;

    /**
     * @brief Compute the complex FFT from audio samples
     * @param samples Input audio samples (size must be equal to num_bins)
     * @return Vector of complex FFT output (size will be num_bins / 2 + 1)
     *         Output bins represent frequencies: [DC, 1*Fs/N, 2*Fs/N, ..., Nyquist]
     *         Where Fs is the sampling frequency and N is num_bins
     * @throws std::invalid_argument if samples.size() != num_bins
     */
    virtual std::vector<fftwf_complex> compute_complex(const std::span<float>& samples) = 0;

    /**
     * @brief Compute the frequency magnitudes from audio samples
     * @param samples Input audio samples (size must be equal to num_bins)
     * @return Vector of frequency magnitudes (size will be num_bins / 2 + 1)
     *         Output bins represent freqencies: [DC, 1*Fs/N, 2*Fs/N, ..., Nyquist]
     *         Where Fs is the sampling frequency and N is num_bins
     * @throws std::invalid_argument if samples.size() != num_bins
     */
    virtual std::vector<float> compute_magnitudes(const std::span<float>& samples) = 0;
};
