#include <algorithm>
#include <cassert>
#include <cmath>
#include <fft_window.h>
#include <stdexcept>

FFTWindow::FFTWindow(size_t size, FFTWindow::Type type)
  : m_size(size)
  , m_type(type)
  , m_window_coefficients(size)
{
    if (size == 0) {
        throw std::invalid_argument("Window size must be greater than zero");
    }
    compute_window_coefficients();
}

/**
 * @brief Apply the window to the input samples.
 * @param input Input samples.  Size must match window size.
 * @return Windowed data
 */
std::vector<float>
FFTWindow::apply(std::span<const float> input) const
{
    if (input.size() != m_size) {
        throw std::invalid_argument("Input size must match window size " + std::to_string(m_size) +
                                    ", got: " + std::to_string(input.size()));
    }

    std::vector<float> output(m_size);
    for (size_t i = 0; i < m_size; ++i) {
        output[i] = input[i] * m_window_coefficients[i];
    }
    return output;
}

/**
 * @brief Compute the window coefficients based on the selected type and size
 */
void
FFTWindow::compute_window_coefficients()
{
    switch (m_type) {
        case Type::Rectangular:
            std::ranges::fill(m_window_coefficients, 1.0f);
            break;
        case Type::Hann:
            for (size_t i = 0; i < m_size; ++i) {
                m_window_coefficients[i] =
                  0.5f * (1.0f - std::cos((2.0f * std::numbers::pi * i) / (m_size - 1)));
            }
            break;
        default:
            assert(false && "Unsupported window type");
            throw std::logic_error("Unsupported window type");
    }
}
