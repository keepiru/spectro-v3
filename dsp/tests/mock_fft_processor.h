#include <cstddef>
#include <ifft_processor.h>
#include <span>
#include <stdexcept>
#include <vector>

/**
 * @brief Mock implementation of IFFTProcessor for testing
 *
 * This mock processor just returns the sample data as both the real and
 * imaginary parts of the complex FFT output, and as the magnitudes.  It allows
 * predefined results to be set for testing purposes.
 */
class MockFFTProcessor : public IFFTProcessor
{
  public:
    /**
     * @brief Constructor
     */
    MockFFTProcessor(uint32_t transform_size)
      : m_transform_size(transform_size)
    {
    }

    uint32_t getTransformSize() const noexcept override { return m_transform_size; }

    /**
     * @brief Return predefined complex FFT results
     * @param samples Input audio samples (size must be equal to transform_size).
     * @return Vector of complex FFT output
     * @throws std::invalid_argument if samples.size() != transform_size
     */
    std::vector<fftwf_complex> compute_complex(const std::span<float>& samples) override
    {
        if (samples.size() != m_transform_size) {
            throw std::invalid_argument("Input sample size does not match transform size");
        }

        std::vector<fftwf_complex> ret(m_transform_size / 2 + 1);
        for (size_t i = 0; i < ret.size(); ++i) {
            ret[i][0] = samples[i]; // Real part
            ret[i][1] = samples[i]; // Imaginary part
        }
        return ret;
    }

    /**
     * @brief Return predefined magnitude results
     * @param samples Input audio samples (size must be equal to transform_size).  Ignored in this
     * mock.
     * @return Vector of frequency magnitudes
     * @throws std::invalid_argument if samples.size() != transform_size
     */
    std::vector<float> compute_magnitudes(const std::span<float>& samples) override
    {
        if (samples.size() != m_transform_size) {
            throw std::invalid_argument("Input sample size does not match transform size");
        }

        std::vector<float> ret(m_transform_size / 2 + 1);
        std::copy(samples.begin(), samples.begin() + ret.size(), ret.begin());
        return ret;
    }

  private:
    uint32_t m_transform_size;
};
