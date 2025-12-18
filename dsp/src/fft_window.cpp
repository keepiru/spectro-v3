#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <fft_window.h>
#include <numbers>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

FFTWindow::FFTWindow(size_t aSize, FFTWindow::Type aType)
  : mSize(aSize)
  , mType(aType)
  , mWindowCoefficients(aSize)
{
    if (aSize == 0) {
        throw std::invalid_argument("Window size must be greater than zero");
    }
    ComputeWindowCoefficients();
}

/**
 * @brief Apply the window to the input samples.
 * @param input Input samples.  Size must match window size.
 * @return Windowed data
 */
std::vector<float>
FFTWindow::Apply(std::span<const float> aInputSamples) const
{
    if (aInputSamples.size() != mSize) {
        throw std::invalid_argument("Input size must match window size " + std::to_string(mSize) +
                                    ", got: " + std::to_string(aInputSamples.size()));
    }

    std::vector<float> output(mSize);
    for (size_t i = 0; i < mSize; ++i) {
        output[i] = aInputSamples[i] * mWindowCoefficients[i];
    }
    return output;
}

/**
 * @brief Compute the window coefficients based on the selected type and size
 */
void
FFTWindow::ComputeWindowCoefficients()
{
    switch (mType) {
        case Type::Rectangular:
            std::ranges::fill(mWindowCoefficients, 1.0f);
            break;
        case Type::Hann:
            for (size_t i = 0; i < mSize; ++i) {
                auto const kPi = std::numbers::pi_v<float>;
                mWindowCoefficients[i] =
                  // The constants here are part of the kHann window definition.
                  // NOLINTNEXTLINE(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
                  0.5f * (1.0f - std::cosf((2.0f * kPi * static_cast<float>(i)) /
                                           static_cast<float>(mSize - 1)));
            }
            break;
        default:
            assert(false && "Unsupported window type");
            throw std::logic_error("Unsupported window type");
    }
}
