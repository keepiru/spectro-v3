// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include <audio_types.h>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <fft_window.h>
#include <numbers>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

FFTWindow::FFTWindow(FFTSize aSize, FFTWindow::Type aType)
  : mSize(aSize)
  , mType(aType)
{
    mWindowCoefficients.resize(aSize);
    ComputeWindowCoefficients();
}

/// @brief Apply the window to the input samples.
/// @param input Input samples.  Size must match window size.
/// @return Windowed data
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

/// @brief Compute the window coefficients based on the selected type and size
void
FFTWindow::ComputeWindowCoefficients()
{
    constexpr float kPi = std::numbers::pi_v<float>;
    const auto kSizeAsFloat = static_cast<float>(mSize);

    // The inline constants are based on standard definitions of the window
    // functions.  We will keep them inline so the formulas are recognizable.
    // NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
    for (size_t i = 0; i < mSize; ++i) {
        const auto kIndexAsFloat = static_cast<float>(i);
        const auto kAngle = (2.0f * kPi * kIndexAsFloat) / kSizeAsFloat;

        switch (mType) {
            case Type::Rectangular:
                mWindowCoefficients[i] = 1.0f;
                break;
            case Type::Hann:
                mWindowCoefficients[i] = 0.5f * (1.0f - std::cosf(kAngle));
                break;
            case Type::Hamming:
                mWindowCoefficients[i] = 0.54f - 0.46f * std::cosf(kAngle);
                break;
            case Type::Blackman:
                mWindowCoefficients[i] =
                  0.42f - 0.5f * std::cosf(kAngle) + 0.08f * std::cosf(2.0f * kAngle);
                break;
            case Type::BlackmanHarris: // 4-term Blackman-Harris window
                mWindowCoefficients[i] = 0.35875f - 0.48829f * std::cosf(kAngle) +
                                         0.14128f * std::cosf(2.0f * kAngle) -
                                         0.01168f * std::cosf(3.0f * kAngle);
                break;
            default:
                assert(false && "Unsupported window type");
                throw std::logic_error("Unsupported window type");
        }
    }
    // NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
}
