// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once
#include <audio_types.h>
#include <functional>
#include <memory>
#include <span>
#include <vector>

struct fftwf_plan_s;
using FftwfPlan = fftwf_plan_s*;

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
    /// @brief Factory function type that creates IFFTProcessor instances with a
    /// specified transform size.
    ///
    /// The function takes a transform size (number of input samples) and returns a
    /// std::unique_ptr<IFFTProcessor> configured for that size.
    using Factory = std::function<std::unique_ptr<IFFTProcessor>(FFTSize)>;

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
    /// @note Zero magnitudes will produce -inf dB values.
    [[nodiscard]] virtual std::vector<float> ComputeDecibels(
      const std::span<float>& aSamples) const = 0;
};

/// @brief Processes audio samples using FFT to produce frequency spectrum
class FFTProcessor : public IFFTProcessor
{
  public:
    /// @brief Constructor
    /// @param aTransformSize FFT transform size (number of input samples, must be power of 2)
    /// @throws std::invalid_argument if aTransformSize is not a power of 2
    explicit FFTProcessor(FFTSize aTransformSize);

    ~FFTProcessor() override = default;

    // Rule of five
    FFTProcessor(const FFTProcessor&) = delete;
    FFTProcessor& operator=(const FFTProcessor&) = delete;
    FFTProcessor(FFTProcessor&&) = default;
    FFTProcessor& operator=(FFTProcessor&&) = default;

    [[nodiscard]] FFTSize GetTransformSize() const noexcept override { return mTransformSize; }
    [[nodiscard]] std::vector<FftwfComplex> ComputeComplex(
      const std::span<float>& aSamples) const override;
    [[nodiscard]] std::vector<float> ComputeMagnitudes(
      const std::span<float>& aSamples) const override;
    [[nodiscard]] std::vector<float> ComputeDecibels(
      const std::span<float>& aSamples) const override;

  private:
    // Custom deleter for FFTW resources (implementation in .cpp)
    struct FFTWDeleter
    {
        void operator()(FftwfPlan plan) const;
        void operator()(float* ptr) const;
        void operator()(FftwfComplex* ptr) const;
    };

    FFTSize mTransformSize;
    using FFTWPlanPtr = std::unique_ptr<std::remove_pointer_t<FftwfPlan>, FFTWDeleter>;
    using FFTWRealPtr = std::unique_ptr<float, FFTWDeleter>;
    using FFTWComplexPtr = std::unique_ptr<FftwfComplex, FFTWDeleter>;
    FFTWPlanPtr mFFTPlan;
    FFTWRealPtr mFFTInput;
    FFTWComplexPtr mFFTOutput;

    void Compute(const std::span<float>& aSamples) const;
};