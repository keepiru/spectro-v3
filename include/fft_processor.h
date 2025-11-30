#pragma once
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

struct fftwf_plan_s;
using fftwf_plan = fftwf_plan_s*;
using fftwf_complex = float[2];

/**
 * @brief Processes audio samples using FFT to produce frequency spectrum
 */
class FFTProcessor
{
  public:
    /**
     * @brief Constructor
     * @param num_bins Number of frequency bins for the FFT (must be power of 2)
     * @throws std::invalid_argument if num_bins is not a power of 2
     */
    explicit FFTProcessor(uint32_t num_bins);

    /**
     * @brief Destructor
     */
    ~FFTProcessor() = default;

    // Rule of five
    FFTProcessor(const FFTProcessor&) = delete;
    FFTProcessor& operator=(const FFTProcessor&) = delete;
    FFTProcessor(FFTProcessor&&) = default;
    FFTProcessor& operator=(FFTProcessor&&) = default;

    /**
     * @brief Get the number of frequency bins
     * @return Number of frequency bins configured for this processor
     */
    uint32_t getNumBins() const noexcept { return m_num_bins; }

    /**
     * @brief Compute the complex FFT from audio samples
     * @param samples Input audio samples (size must be equal to num_bins)
     * @return Vector of complex FFT output (size will be num_bins / 2 + 1)
     * @throws std::invalid_argument if samples.size() != num_bins
     */
    std::vector<fftwf_complex> compute_complex(const std::span<float>& samples);

    /**
     * @brief Compute the frequency magnitudes from audio samples
     * @param samples Input audio samples (size must be equal to num_bins)
     * @return Vector of frequency magnitudes (size will be num_bins / 2 + 1)
     * @throws std::invalid_argument if samples.size() != num_bins
     */
    std::vector<float> compute_magnitudes(const std::span<float>& samples);

  private:
    // Custom deleter for FFTW resources (implementation in .cpp)
    struct FFTWDeleter
    {
        void operator()(fftwf_plan plan) const;
        void operator()(float* ptr) const;
        void operator()(fftwf_complex* ptr) const;
    };

    uint32_t m_num_bins;
    using FFTWPlanPtr = std::unique_ptr<std::remove_pointer<fftwf_plan>::type, FFTWDeleter>;
    using FFTWRealPtr = std::unique_ptr<float, FFTWDeleter>;
    using FFTWComplexPtr = std::unique_ptr<fftwf_complex, FFTWDeleter>;
    FFTWPlanPtr m_fft_plan;
    FFTWRealPtr m_fft_input;
    FFTWComplexPtr m_fft_output;

    void compute(const std::span<float>& samples);
};