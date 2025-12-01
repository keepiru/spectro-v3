#include <cstddef>
#include <ifft_processor.h>
#include <span>
#include <stdexcept>
#include <vector>

/**
 * @brief Mock implementation of IFFTProcessor for testing
 *
 * This mock processor returns predefined results for FFT computations.
 */
class MockFFTProcessor : public IFFTProcessor
{
  public:
    /**
     * @brief Constructor
     */
    MockFFTProcessor(uint32_t bins)
      : m_num_bins(bins)
      , m_results()
    {
    }

    /**
     * @brief add a predefined result to return
     * @param result The result vector to add
     */
    void addResult(const std::vector<float>& result) { m_results.push_back(result); }

    uint32_t getNumBins() const noexcept override { return m_num_bins; }

    /**
     * @brief Return predefined complex FFT results
     * @param samples Input audio samples (size must be equal to num_bins).  Ignored in this mock.
     * @return Vector of complex FFT output
     * @throws std::invalid_argument if samples.size() != num_bins
     */
    std::vector<fftwf_complex> compute_complex(const std::span<float>& samples) override
    {
        if (samples.size() != m_num_bins) {
            throw std::invalid_argument("Input sample size does not match number of bins");
        }

        auto result = m_results.front();
        m_results.erase(m_results.begin());

        std::vector<fftwf_complex> ret(result.size());
        for (size_t i = 0; i < result.size(); ++i) {
            ret[i][0] = result[i]; // Real part
            ret[i][1] = result[i]; // Imaginary part
        }
        return ret;
    }

    /**
     * @brief Return predefined magnitude results
     * @param samples Input audio samples (size must be equal to num_bins).  Ignored in this mock.
     * @return Vector of frequency magnitudes
     * @throws std::invalid_argument if samples.size() != num_bins
     */
    std::vector<float> compute_magnitudes(const std::span<float>& samples) override
    {
        if (samples.size() != m_num_bins) {
            throw std::invalid_argument("Input sample size does not match number of bins");
        }

        auto ret = m_results.front();
        m_results.erase(m_results.begin());
        return ret;
    }

  private:
    uint32_t m_num_bins;
    std::vector<std::vector<float>> m_results;
};
