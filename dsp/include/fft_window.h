#pragma once
#include <audio_types.h>
#include <cstdint>
#include <functional>
#include <memory>
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
     * @param aSize Number of samples in the window (must be > 0)
     * @param aType Window function type
     * @throws std::invalid_argument if aSize is not positive
     *
     * Window functions are precomputed upon construction for performance.
     */
    FFTWindow(FFTSize aSize, Type aType);

    /**
     * @brief Apply window to samples, returning windowed data
     * @param aInput Input samples.  Size must match window size
     * @return Windowed samples
     * @throws std::invalid_argument if aInput.size() != window size
     */
    [[nodiscard]] std::vector<float> Apply(std::span<const float> aInput) const;

    /**
     * @brief Get the size of the window
     * @return Window size in samples
     */
    [[nodiscard]] FFTSize GetSize() const noexcept { return mSize; }

    /**
     * @brief Get the type of the window
     * @return Window type
     */
    [[nodiscard]] Type GetType() const noexcept { return mType; }

  private:
    FFTSize mSize;                          // Window size in samples
    Type mType;                             // Window type
    std::vector<float> mWindowCoefficients; // Precomputed window coefficients

    /**
     * @brief Compute the window coefficients based on the selected type and size
     */
    void ComputeWindowCoefficients();
};

/**
 * @brief Factory function type that creates FFTWindow instances with a
 * specified size and type.
 *
 * The function takes a size (number of samples) and a window type, and returns
 * a std::unique_ptr<FFTWindow> configured for that size and type.
 */
using FFTWindowFactory = std::function<std::unique_ptr<FFTWindow>(FFTSize, FFTWindow::Type)>;