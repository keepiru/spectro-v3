#pragma once
#include <cstdint>
#include <span>
#include <vector>

class FFTWindow
{
  public:
    enum class Type : uint8_t
    {
        Rectangular,
        Hann,
    };

    /**
     * @brief Constructor
     * @param size Number of samples in the window (must be > 0)
     * @param type Window function type
     * @throws std::invalid_argument if size is zero
     *
     * Window functions are precomputed upon construction for performance.
     */
    FFTWindow(size_t size, Type type);

    /**
     * @brief Apply window to samples, returning windowed data
     * @param input Input samples.  Size must match window size
     * @return Windowed samples
     * @throws std::invalid_argument if input.size() != window size
     */
    [[nodiscard]] std::vector<float> apply(std::span<const float> input) const;

    /**
     * @brief Get the size of the window
     * @return Window size in samples
     */
    [[nodiscard]] size_t getSize() const noexcept { return m_size; }

    /**
     * @brief Get the type of the window
     * @return Window type
     */
    [[nodiscard]] Type getType() const noexcept { return m_type; }

  private:
    size_t m_size;                            // Window size in samples
    Type m_type;                              // Window type
    std::vector<float> m_window_coefficients; // Precomputed window coefficients

    /**
     * @brief Compute the window coefficients based on the selected type and size
     */
    void computeWindowCoefficients();
};
