#include <cmath>
#include <fft_processor.h>
#include <stdexcept>

namespace {
bool
isPowerOf2(uint32_t n)
{
    return (n > 0) && ((n & (n - 1)) == 0);
}
}

FFTProcessor::FFTProcessor(uint32_t num_bins)
  : m_num_bins(num_bins)
  , m_fft_plan(nullptr)
  , m_fft_input(nullptr)
  , m_fft_output(nullptr)
{
    if (!isPowerOf2(num_bins)) {
        throw std::invalid_argument("num_bins must be a power of 2, got: " +
                                    std::to_string(num_bins));
    }

    m_fft_input = FFTWRealPtr(fftwf_alloc_real(m_num_bins));
    m_fft_output = FFTWComplexPtr(fftwf_alloc_complex(m_num_bins / 2 + 1));
    m_fft_plan = FFTWPlanPtr(
      fftwf_plan_dft_r2c_1d(m_num_bins, m_fft_input.get(), m_fft_output.get(), FFTW_ESTIMATE));

    if (!m_fft_plan) {
        throw std::runtime_error("Failed to create FFTW plan");
    }
}

void
FFTProcessor::compute(const std::vector<float>& samples)
{
    if (samples.size() != m_num_bins) {
        throw std::invalid_argument("Input samples size must be equal to num_bins");
    }
    // Copy input samples to FFTW input buffer
    std::copy(samples.begin(), samples.end(), m_fft_input.get());
    // Execute the FFT
    fftwf_execute(m_fft_plan.get());
}

std::vector<fftwf_complex>
FFTProcessor::compute_complex(const std::vector<float>& samples)
{
    compute(samples);

    std::vector<fftwf_complex> output(m_num_bins / 2 + 1);
    auto fft_output_ptr = m_fft_output.get();
    for (uint32_t i = 0; i < output.size(); ++i) {
        output[i][0] = fft_output_ptr[i][0];
        output[i][1] = fft_output_ptr[i][1];
    }
    return output;
}

std::vector<float>
FFTProcessor::compute_magnitudes(const std::vector<float>& samples)
{
    compute(samples);

    std::vector<float> magnitudes(m_num_bins / 2 + 1);
    for (uint32_t i = 0; i < magnitudes.size(); ++i) {
        float real = m_fft_output.get()[i][0];
        float imag = m_fft_output.get()[i][1];
        magnitudes[i] = std::sqrt(real * real + imag * imag);
    }
    return magnitudes;
}