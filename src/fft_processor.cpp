#include <fft_processor.h>
#include <stdexcept>

bool
isPowerOf2(uint32_t n)
{
  return (n > 0) && ((n & (n - 1)) == 0);
}

FFTProcessor::FFTProcessor(uint32_t num_bins)
{
  if (!isPowerOf2(num_bins)) {
    throw std::invalid_argument("num_bins must be a power of 2, got: " + std::to_string(num_bins));
  }
  m_num_bins = num_bins;
}