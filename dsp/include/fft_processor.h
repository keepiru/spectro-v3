// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once
#include <audio_types.h>
#include <ifft_processor.h>
#include <memory>
#include <span>
#include <vector>

struct fftwf_plan_s;
using FftwfPlan = fftwf_plan_s*;

// We have to match the FFTW complex type definition
using FftwfComplex = float[2]; // NOLINT (cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)

/// @brief Processes audio samples using FFT to produce frequency spectrum
class FFTProcessor : public IFFTProcessor
{
  public:
    /// @brief Constructor
    /// @param aTransformSize FFT transform size (number of input samples, must be power of 2)
    /// @throws std::invalid_argument if aTransformSize is not a power of 2
    explicit FFTProcessor(FFTSize aTransformSize);

    /// @brief Destructor
    ~FFTProcessor() override = default;

    // Rule of five
    FFTProcessor(const FFTProcessor&) = delete;
    FFTProcessor& operator=(const FFTProcessor&) = delete;
    FFTProcessor(FFTProcessor&&) = default;
    FFTProcessor& operator=(FFTProcessor&&) = default;

    /// @brief Get the FFT transform size
    /// @return Transform size (number of input samples) configured for this processor
    [[nodiscard]] FFTSize GetTransformSize() const noexcept override { return mTransformSize; }

    /// @brief Compute the complex FFT from audio samples
    /// @param aSamples Input audio samples (size must be equal to transform_size)
    /// @return Vector of complex FFT output (size will be transform_size / 2 + 1)
    ///         Output bins represent frequencies: [DC, 1*Fs/N, 2*Fs/N, ..., Nyquist]
    ///         Where Fs is the sampling frequency and N is transform_size
    /// @throws std::invalid_argument if aSamples.size() != transform_size
    [[nodiscard]] std::vector<FftwfComplex> ComputeComplex(
      const std::span<float>& aSamples) const override;

    /// @brief Compute the frequency magnitudes from audio samples
    /// @param aSamples Input audio aSamples (size must be equal to transform_size)
    /// @return Vector of frequency magnitudes (size will be transform_size / 2 + 1)
    ///         Output bins represent frequencies: [DC, 1*Fs/N, 2*Fs/N, ..., Nyquist]
    ///         Where Fs is the sampling frequency and N is transform_size
    /// @throws std::invalid_argument if aSamples.size() != transform_size
    [[nodiscard]] std::vector<float> ComputeMagnitudes(
      const std::span<float>& aSamples) const override;

    /// @brief Compute the frequency magnitudes in decibels from audio samples
    /// @param aSamples Input audio aSamples (size must be equal to transform_size)
    /// @return Vector of frequency magnitudes in decibels (size will be transform_size / 2 + 1)
    ///         Output bins represent frequencies: [DC, 1*Fs/N, 2*Fs/N, ..., Nyquist]
    ///         Where Fs is the sampling frequency and N is transform_size
    /// @throws std::invalid_argument if aSamples.size() != transform_size
    /// @note Zero magnitudes will produce -inf dB values.
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