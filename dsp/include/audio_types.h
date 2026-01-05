#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <sndfile.h>
#include <stdexcept>

using ChannelCount = uint8_t;

// === Tag types for strong typedefs ===
// Used to differentiate Count types for frames vs. samples
struct TagFrame
{};
struct TagSample
{};

template<typename T, typename Tag>
class Count
{
  public:
    // Default constructor for Qt metatype system
    constexpr Count()
      : mValue(0)
    {
    }

    explicit constexpr Count(T aValue)
      : mValue(aValue)
    {
    }

    /// @brief Get the underlying count value
    [[nodiscard]] constexpr T Get() const noexcept { return mValue; }

    [[nodiscard]] constexpr bool operator==(Count<T, Tag> aOther) const noexcept
    {
        return mValue == aOther.Get();
    }

  private:
    T mValue;
};

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

/// Offset between frame positions (can be negative)
using FrameOffset = int64_t;

/// Count of frames (always non-negative)
class FrameCount : public Count<size_t, TagFrame>
{
  public:
    // inherit constructors
    using Count<size_t, TagFrame>::Count;

    /// @brief Multiply frame count by channel count to get total sample count
    /// @param aChannels The number of audio channels
    /// @return The total sample count across all channels
    [[nodiscard]] constexpr SampleCount operator*(ChannelCount aChannels) const noexcept
    {
        return static_cast<SampleCount>(Get()) * aChannels;
    }

    /// @brief Cast to int with overflow check
    /// @return The frame count as int
    /// @throws std::overflow_error if the count exceeds int maximum
    /// Used to convert to int for scrollbar maximums
    [[nodiscard]] constexpr int ToIntChecked() const
    {
        if (Get() > std::numeric_limits<int>::max()) {
            throw std::overflow_error(std::format("Count({}) exceeds int max", Get()));
        }
        return static_cast<int>(Get());
    }

    /// Convert to sf_count_t for libsndfile API compatibility
    [[nodiscard]] constexpr sf_count_t ToSfCountT() const noexcept
    {
        return static_cast<sf_count_t>(Get());
    }

    [[nodiscard]] constexpr FrameOffset AsOffset() const noexcept
    {
        return static_cast<FrameOffset>(Get());
    }
};

/// Index into audio timeline (0-based frame position)
// Currently unused
// using FrameIndex = size_t;

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
