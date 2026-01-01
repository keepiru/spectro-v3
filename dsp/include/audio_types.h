#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>

/// Audio sample rate in Hz (e.g., 44100, 48000)
/// int is used for compatibility with FFTW3 and libsndfile.
using SampleRate = int;

/// FFT transform size (must be a power of 2)
/// int is used for compatibility with FFTW3.
using FFTSize = int;

// === Sample types (single channel values) ===

/// Count of samples (always non-negative)
using SampleCount = size_t;

/// Index into a sample buffer (0-based, non-negative position)
using SampleIndex = size_t;

/// Offset between sample positions (can be negative for backward references)
using SampleOffset = int64_t;

// === Frame types (multi-channel time positions) ===
// A frame represents one point in time across all channels.
// In mono: 1 frame = 1 sample. In stereo: 1 frame = 2 samples.

/// Count of frames (always non-negative)
using FrameCount = size_t;

/// Index into audio timeline (0-based frame position)
using FrameIndex = size_t;

/// Offset between frame positions (can be negative)
using FrameOffset = int64_t;

// === Conversion helpers ===

/// Safely convert a signed sample offset to unsigned index.
/// @param aOffset The signed offset to convert
/// @return The offset as an unsigned SampleIndex
/// @throws std::out_of_range if aOffset is negative
[[nodiscard]] inline SampleIndex
ToSampleIndex(SampleOffset aOffset)
{
    if (aOffset < 0) {
        throw std::out_of_range("Negative sample offset cannot be converted to index");
    }
    return static_cast<SampleIndex>(aOffset);
}

/// Safely convert a signed sample offset to unsigned index, clamping negatives to 0.
/// @param aOffset The signed offset to convert
/// @return The offset as an unsigned SampleIndex, or 0 if negative
[[nodiscard]] constexpr SampleIndex
ToSampleIndexClamped(SampleOffset aOffset) noexcept
{
    return aOffset < 0 ? 0 : static_cast<SampleIndex>(aOffset);
}

/// Safely convert a signed frame offset to unsigned index.
/// @param aOffset The signed offset to convert
/// @return The offset as an unsigned FrameIndex
/// @throws std::out_of_range if aOffset is negative
[[nodiscard]] inline FrameIndex
ToFrameIndex(FrameOffset aOffset)
{
    if (aOffset < 0) {
        throw std::out_of_range("Negative frame offset cannot be converted to index");
    }
    return static_cast<FrameIndex>(aOffset);
}

/// Safely convert a signed frame offset to unsigned index, clamping negatives to 0.
/// @param aOffset The signed offset to convert
/// @return The offset as an unsigned FrameIndex, or 0 if negative
[[nodiscard]] constexpr FrameIndex
ToFrameIndexClamped(FrameOffset aOffset) noexcept
{
    return aOffset < 0 ? 0 : static_cast<FrameIndex>(aOffset);
}

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
