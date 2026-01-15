#pragma once
#include <audio_types.h>
#include <functional>
#include <memory>
#include <span>
#include <vector>

// Forward declaration for FFTW complex type
// We have to match the FFTW complex type definition
using FftwfComplex = float[2]; // NOLINT (cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)

/// @brief Interface for FFT processors
///
/// Pure virtual interface for computing FFT operations on audio samples.
/// Enables dependency injection and mock implementations for testing.
class IFFTProcessor
{
  public:
    virtual ~IFFTProcessor() = default;

    /// @brief Get the FFT transform size
    /// @return Transform size (number of input samples) configured for this processor
    [[nodiscard]] virtual FFTSize GetTransformSize() const noexcept = 0;

    /// @brief Compute the complex FFT from audio samples
    /// @param aSamples Input audio samples (size must be equal to transform_size)
    /// @return Vector of complex FFT output (size will be transform_size / 2 + 1)
    ///         Output bins represent frequencies: [DC, 1*Fs/N, 2*Fs/N, ..., Nyquist]
    ///         Where Fs is the sampling frequency and N is transform_size
    /// @throws std::invalid_argument if aSamples.size() != transform_size
    [[nodiscard]] virtual std::vector<FftwfComplex> ComputeComplex(
      const std::span<float>& aSamples) const = 0;

    /// @brief Compute the frequency magnitudes from audio samples
    /// @param aSamples Input audio samples (size must be equal to transform_size)
    /// @return Vector of frequency magnitudes (size will be transform_size / 2 + 1)
    ///         Output bins represent frequencies: [DC, 1*Fs/N, 2*Fs/N, ..., Nyquist]
    ///         Where Fs is the sampling frequency and N is transform_size
    /// @throws std::invalid_argument if aSamples.size() != transform_size
    [[nodiscard]] virtual std::vector<float> ComputeMagnitudes(
      const std::span<float>& aSamples) const = 0;

    /// @brief Compute the frequency magnitudes in decibels from audio samples
    /// @param aSamples Input audio samples (size must be equal to transform_size)
    /// @return Vector of frequency magnitudes in decibels (size will be transform_size / 2 + 1)
    ///         Output bins represent frequencies: [DC, 1*Fs/N, 2*Fs/N, ..., Nyquist]
    ///         Where Fs is the sampling frequency and N is transform_size
    /// @throws std::invalid_argument if aSamples.size() != transform_size
    [[nodiscard]] virtual std::vector<float> ComputeDecibels(
      const std::span<float>& aSamples) const = 0;
};

/// @brief Factory function type that creates IFFTProcessor instances with a
/// specified transform size.
///
/// The function takes a transform size (number of input samples) and returns a
/// std::unique_ptr<IFFTProcessor> configured for that size.
using IFFTProcessorFactory = std::function<std::unique_ptr<IFFTProcessor>(FFTSize)>;