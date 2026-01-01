#pragma once

/// Audio sample rate in Hz (e.g., 44100, 48000)
/// int is used for compatibility with FFTW3 and libsndfile.
using SampleRate = int;

/// FFT transform size (must be a power of 2)
/// int is used for compatibility with FFTW3.
using FFTSize = int;

/// Validates that a value is a power of 2.
/// This helper is intended for checking FFTSize values. Although FFTSize is
/// a signed type (for FFTW3 compatibility), negative values are considered
/// invalid and will always return false from this function.
/// @param n The value to check (typically a non-negative FFTSize)
/// @return true if n is a positive power of 2, false otherwise
[[nodiscard]] constexpr bool
IsPowerOf2(FFTSize n) noexcept
{
    return (n > 0) && ((n & (n - 1)) == 0);
}
