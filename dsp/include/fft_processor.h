#pragma once
#include <cstdint>
#include <ifft_processor.h>
#include <memory>
#include <span>
#include <vector>

struct fftwf_plan_s;
using fftwf_plan = fftwf_plan_s*;

// We have to match the FFTW complex type definition
using fftwf_complex =
  float[2]; // NOLINT (cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)

/**
 * @brief Processes audio samples using FFT to produce frequency spectrum
 */
class FFTProcessor : public IFFTProcessor
{
  public:
    /**
     * @brief Constructor
     * @param aTransformSize FFT transform size (number of input samples, must be power of 2)
     * @throws std::invalid_argument if aTransformSize is not a power of 2
     */
    explicit FFTProcessor(int32_t aTransformSize);

    /**
     * @brief Destructor
     */
    ~FFTProcessor() override = default;

    // Rule of five
    FFTProcessor(const FFTProcessor&) = delete;
    FFTProcessor& operator=(const FFTProcessor&) = delete;
    FFTProcessor(FFTProcessor&&) = default;
    FFTProcessor& operator=(FFTProcessor&&) = default;

    /**
     * @brief Get the FFT transform size
     * @return Transform size (number of input samples) configured for this processor
     */
    [[nodiscard]] uint32_t GetTransformSize() const noexcept override { return mTransformSize; }

    /**
     * @brief Compute the complex FFT from audio samples
     * @param aSamples Input audio samples (size must be equal to transform_size)
     * @return Vector of complex FFT output (size will be transform_size / 2 + 1)
     *         Output bins represent frequencies: [DC, 1*Fs/N, 2*Fs/N, ..., Nyquist]
     *         Where Fs is the sampling frequency and N is transform_size
     * @throws std::invalid_argument if aSamples.size() != transform_size
     */
    [[nodiscard]] std::vector<fftwf_complex> ComputeComplex(
      const std::span<float>& aSamples) const override;

    /**
     * @brief Compute the frequency magnitudes from audio samples
     * @param aSamples Input audio aSamples (size must be equal to transform_size)
     * @return Vector of frequency magnitudes (size will be transform_size / 2 + 1)
     *         Output bins represent frequencies: [DC, 1*Fs/N, 2*Fs/N, ..., Nyquist]
     *         Where Fs is the sampling frequency and N is transform_size
     * @throws std::invalid_argument if aSamples.size() != transform_size
     */
    [[nodiscard]] std::vector<float> ComputeMagnitudes(
      const std::span<float>& aSamples) const override;

  private:
    // Custom deleter for FFTW resources (implementation in .cpp)
    struct FFTWDeleter
    {
        void operator()(fftwf_plan plan) const;
        void operator()(float* ptr) const;
        void operator()(fftwf_complex* ptr) const;
    };

    int32_t mTransformSize;
    using FFTWPlanPtr = std::unique_ptr<std::remove_pointer_t<fftwf_plan>, FFTWDeleter>;
    using FFTWRealPtr = std::unique_ptr<float, FFTWDeleter>;
    using FFTWComplexPtr = std::unique_ptr<fftwf_complex, FFTWDeleter>;
    FFTWPlanPtr mFFTPlan;
    FFTWRealPtr mFFTInput;
    FFTWComplexPtr mFFTOutput;

    void Compute(const std::span<float>& aSamples) const;
};