// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "audio_types.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fft_processor.h>
#include <fftw3.h>
#include <span>
#include <stdexcept>
#include <vector>

FFTProcessor::FFTProcessor(FFTSize aTransformSize)
  : mTransformSize(aTransformSize)
  , mFFTPlan(nullptr)
  , mFFTInput(nullptr)
  , mFFTOutput(nullptr)
{
    mFFTInput = FFTWRealPtr(fftwf_alloc_real(mTransformSize));
    mFFTOutput = FFTWComplexPtr(fftwf_alloc_complex((mTransformSize / 2) + 1));
    mFFTPlan = FFTWPlanPtr(
      fftwf_plan_dft_r2c_1d(mTransformSize, mFFTInput.get(), mFFTOutput.get(), FFTW_ESTIMATE));

    if (!mFFTPlan) {
        throw std::runtime_error("Failed to create FFTW plan");
    }
}

void
FFTProcessor::Compute(const std::span<float>& aSamples) const
{
    if (aSamples.size() != mTransformSize) {
        throw std::invalid_argument("Input aSamples size must be equal to transform_size");
    }
    // Copy input aSamples to FFTW input buffer
    std::ranges::copy(aSamples, mFFTInput.get());
    // Execute the FFT
    fftwf_execute(mFFTPlan.get());
}

std::vector<FftwfComplex>
FFTProcessor::ComputeComplex(const std::span<float>& aSamples) const
{
    Compute(aSamples);

    std::vector<FftwfComplex> outputPtr((mTransformSize / 2) + 1);
    std::memcpy(outputPtr.data(), mFFTOutput.get(), outputPtr.size() * sizeof(FftwfComplex));
    return outputPtr;
}

std::vector<float>
FFTProcessor::ComputeMagnitudes(const std::span<float>& aSamples) const
{
    Compute(aSamples);

    std::vector<float> magnitudes((mTransformSize / 2) + 1);
    for (uint32_t i = 0; i < magnitudes.size(); ++i) {
        float const real = mFFTOutput.get()[i][0];
        float const imag = mFFTOutput.get()[i][1];
        magnitudes[i] = std::sqrt((real * real) + (imag * imag));
    }
    return magnitudes;
}

std::vector<float>
FFTProcessor::ComputeDecibels(const std::span<float>& aSamples) const
{
    const std::vector<float> magnitudes = ComputeMagnitudes(aSamples);
    std::vector<float> decibels(magnitudes.size());
    for (size_t i = 0; i < magnitudes.size(); ++i) {
        // Standard conversion: dB = 20 * log10(magnitude)
        constexpr float kDecibelScaleFactor = 20.0F;
        // Note that zero magnitudes will produce -inf dB.  This is correct
        // floating-point behavior.
        decibels[i] = kDecibelScaleFactor * std::log10(magnitudes[i]);
    }
    return decibels;
}

void
FFTProcessor::FFTWDeleter::operator()(FftwfPlan aPlan) const
{
    if (aPlan) {
        fftwf_destroy_plan(aPlan);
    }
}

void
FFTProcessor::FFTWDeleter::operator()(float* aPtr) const
{
    if (aPtr) {
        fftwf_free(aPtr);
    }
}

void
FFTProcessor::FFTWDeleter::operator()(FftwfComplex* aPtr) const
{
    if (aPtr) {
        fftwf_free(aPtr);
    }
}