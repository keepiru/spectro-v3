#pragma once
#include <cstdint>

class FFTProcessor
{
public:
  explicit FFTProcessor(uint32_t num_bins)
    : m_num_bins(num_bins) {};

  uint32_t getNumBins() const { return m_num_bins; }

private:
  uint32_t m_num_bins;
};