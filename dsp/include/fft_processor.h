#pragma once
#include <cstdint>
#include <ifft_processor.h>
#include <memory>
#include <span>
#include <vector>

struct fftwf_plan_s;
using fftwf_plan = fftwf_plan_s*;
using fftwf_complex = float[2];

/**
 * @brief Processes audio samples using FFT to produce frequency spectrum
 */
class FFTProcessor : public IFFTProcessor
{
  public:
    /**
     * @brief Constructor
     * @param transform_size FFT transform size (number of input samples, must be power of 2)
     * @throws std::invalid_argument if transform_size is not a power of 2
     */
    explicit FFTProcessor(uint32_t transform_size);

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
    uint32_t getTransformSize() const noexcept override { return m_transform_size; }

    /**
     * @brief Compute the complex FFT from audio samples
     * @param samples Input audio samples (size must be equal to transform_size)
     * @return Vector of complex FFT output (size will be transform_size / 2 + 1)
     *         Output bins represent frequencies: [DC, 1*Fs/N, 2*Fs/N, ..., Nyquist]
     *         Where Fs is the sampling frequency and N is transform_size
     * @throws std::invalid_argument if samples.size() != transform_size
     */
    std::vector<fftwf_complex> compute_complex(const std::span<float>& samples) override;

    /**
     * @brief Compute the frequency magnitudes from audio samples
     * @param samples Input audio samples (size must be equal to transform_size)
     * @return Vector of frequency magnitudes (size will be transform_size / 2 + 1)
     *         Output bins represent frequencies: [DC, 1*Fs/N, 2*Fs/N, ..., Nyquist]
     *         Where Fs is the sampling frequency and N is transform_size
     * @throws std::invalid_argument if samples.size() != transform_size
     */
    std::vector<float> compute_magnitudes(const std::span<float>& samples) override;

  private:
    // Custom deleter for FFTW resources (implementation in .cpp)
    struct FFTWDeleter
    {
        void operator()(fftwf_plan plan) const;
        void operator()(float* ptr) const;
        void operator()(fftwf_complex* ptr) const;
    };

    uint32_t m_transform_size;
    using FFTWPlanPtr = std::unique_ptr<std::remove_pointer<fftwf_plan>::type, FFTWDeleter>;
    using FFTWRealPtr = std::unique_ptr<float, FFTWDeleter>;
    using FFTWComplexPtr = std::unique_ptr<fftwf_complex, FFTWDeleter>;
    FFTWPlanPtr m_fft_plan;
    FFTWRealPtr m_fft_input;
    FFTWComplexPtr m_fft_output;

    void compute(const std::span<float>& samples);
};