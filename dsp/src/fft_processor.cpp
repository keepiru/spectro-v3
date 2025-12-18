#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fft_processor.h>
#include <fftw3.h>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
bool
IsPowerOf2(uint32_t n)
{
    return (n > 0) && ((n & (n - 1)) == 0);
}
}

FFTProcessor::FFTProcessor(int32_t aTransformSize)
  : mTransformSize(aTransformSize)
  , mFFTPlan(nullptr)
  , mFFTInput(nullptr)
  , mFFTOutput(nullptr)
{
    if (!IsPowerOf2(aTransformSize)) {
        throw std::invalid_argument("aTransformSize must be a power of 2, got: " +
                                    std::to_string(aTransformSize));
    }

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