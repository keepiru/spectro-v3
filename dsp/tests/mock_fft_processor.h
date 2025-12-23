#pragma once
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
    MockFFTProcessor(uint32_t aTransformSize)
      : mTransformSize(aTransformSize)
    {
    }

    [[nodiscard]] uint32_t GetTransformSize() const noexcept override { return mTransformSize; }

    /**
     * @brief Return predefined complex FFT results
     * @param aInputSamples Input audio aInputSamples (size must be equal to transform_size).
     * @return Vector of complex FFT output
     * @throws std::invalid_argument if aInputSamples.size() != transform_size
     */
    [[nodiscard]] std::vector<FftwfComplex> ComputeComplex(
      const std::span<float>& aInputSamples) const override
    {
        if (aInputSamples.size() != mTransformSize) {
            throw std::invalid_argument("Input sample size does not match transform size");
        }

        std::vector<FftwfComplex> ret((mTransformSize / 2) + 1);
        for (size_t i = 0; i < ret.size(); ++i) {
            ret[i][0] = aInputSamples[i]; // Real part
            ret[i][1] = aInputSamples[i]; // Imaginary part
        }
        return ret;
    }

    /**
     * @brief Return predefined magnitude results
     * @param aInputSamples Input audio aInputSamples (size must be equal to transform_size).
     * Ignored in this mock.
     * @return Vector of frequency magnitudes
     * @throws std::invalid_argument if aInputSamples.size() != transform_size
     */
    [[nodiscard]] std::vector<float> ComputeMagnitudes(
      const std::span<float>& aInputSamples) const override
    {
        if (aInputSamples.size() != mTransformSize) {
            throw std::invalid_argument("Input sample size does not match transform size");
        }

        std::vector<float> ret((mTransformSize / 2) + 1);
        std::copy(aInputSamples.begin(),
                  aInputSamples.begin() + static_cast<std::ptrdiff_t>(ret.size()),
                  ret.begin());
        return ret;
    }

    [[nodiscard]] std::vector<float> ComputeDecibels(
      const std::span<float>& aInputSamples) const override
    {
        return ComputeMagnitudes(aInputSamples);
    }

  private:
    uint32_t mTransformSize;
};
