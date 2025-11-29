#pragma once
#include <cstdint>

// @brief Processes audio samples using FFT to produce frequency spectrum
class FFTProcessor
{
public:
  // @brief Constructor
  // @param num_bins Number of frequency bins for the FFT (must be power of 2)
  // @throws std::invalid_argument if num_bins is not a power of 2
  explicit FFTProcessor(uint32_t num_bins);

  // @brief Get the number of frequency bins
  // @return Number of frequency bins configured for this processor
  uint32_t getNumBins() const { return m_num_bins; }

private:
  uint32_t m_num_bins;
};