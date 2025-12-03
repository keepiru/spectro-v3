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
isPowerOf2(uint32_t n)
{
    return (n > 0) && ((n & (n - 1)) == 0);
}
}

FFTProcessor::FFTProcessor(int32_t transform_size)
  : m_transform_size(transform_size)
  , m_fft_plan(nullptr)
  , m_fft_input(nullptr)
  , m_fft_output(nullptr)
{
    if (!isPowerOf2(transform_size)) {
        throw std::invalid_argument("transform_size must be a power of 2, got: " +
                                    std::to_string(transform_size));
    }

    m_fft_input = FFTWRealPtr(fftwf_alloc_real(m_transform_size));
    m_fft_output = FFTWComplexPtr(fftwf_alloc_complex((m_transform_size / 2) + 1));
    m_fft_plan = FFTWPlanPtr(fftwf_plan_dft_r2c_1d(
      m_transform_size, m_fft_input.get(), m_fft_output.get(), FFTW_ESTIMATE));

    if (!m_fft_plan) {
        throw std::runtime_error("Failed to create FFTW plan");
    }
}

void
FFTProcessor::compute(const std::span<float>& samples) const
{
    if (samples.size() != m_transform_size) {
        throw std::invalid_argument("Input samples size must be equal to transform_size");
    }
    // Copy input samples to FFTW input buffer
    std::ranges::copy(samples, m_fft_input.get());
    // Execute the FFT
    fftwf_execute(m_fft_plan.get());
}

std::vector<fftwf_complex>
FFTProcessor::computeComplex(const std::span<float>& samples) const
{
    compute(samples);

    std::vector<fftwf_complex> output_ptr((m_transform_size / 2) + 1);
    std::memcpy(output_ptr.data(), m_fft_output.get(), output_ptr.size() * sizeof(fftwf_complex));
    return output_ptr;
}

std::vector<float>
FFTProcessor::computeMagnitudes(const std::span<float>& samples) const
{
    compute(samples);

    std::vector<float> magnitudes((m_transform_size / 2) + 1);
    for (uint32_t i = 0; i < magnitudes.size(); ++i) {
        float const real = m_fft_output.get()[i][0];
        float const imag = m_fft_output.get()[i][1];
        magnitudes[i] = std::sqrt((real * real) + (imag * imag));
    }
    return magnitudes;
}

void
FFTProcessor::FFTWDeleter::operator()(fftwf_plan plan) const
{
    if (plan) {
        fftwf_destroy_plan(plan);
    }
}

void
FFTProcessor::FFTWDeleter::operator()(float* ptr) const
{
    if (ptr) {
        fftwf_free(ptr);
    }
}

void
FFTProcessor::FFTWDeleter::operator()(fftwf_complex* ptr) const
{
    if (ptr) {
        fftwf_free(ptr);
    }
}