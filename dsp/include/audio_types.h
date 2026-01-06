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

// Forward declarations so Count and Index can reference each other
template<typename T, typename Tag>
class Index;
template<typename T, typename Tag>
class Count;
class FrameCount;
class FramePosition;

/// FFT transform size (must be a power of 2)
/// int is used for compatibility with FFTW3.
using FFTSize = int;

/// Audio sample rate in Hz (e.g., 44100, 48000)
/// int is used for compatibility with FFTW3 and libsndfile.
using SampleRate = int;

/// @brief Represents a count of items of type T, tagged with Tag for strong typing.
/// @tparam T The underlying integer type (e.g., size_t)
/// @tparam Tag A unique tag type to differentiate between different count types
template<typename T, typename Tag>
class Count
{
  public:
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

    /// @brief Cast to ptrdiff_t for use in iterator arithmetic
    /// @return The count as ptrdiff_t
    [[nodiscard]] constexpr std::ptrdiff_t AsPtrDiffT() const noexcept
    {
        return static_cast<std::ptrdiff_t>(mValue);
    }

  private:
    T mValue;
};

/// @brief Represents a position (index) into a sequence of items of type T, tagged with Tag
///        for strong typing.
///
/// Unlike Count<T, Tag>, which represents a quantity/size (how many items), Index<T, Tag>
/// represents a zero-based position (where in the sequence). Use Count when you mean "how
/// many" and Index when you mean "at which position". Adding a Count to an Index produces
/// a new Index that refers to a different position.
///
/// @tparam T The underlying integer type (e.g., size_t)
/// @tparam Tag A unique tag type to differentiate between different index types
template<typename T, typename Tag>
class Index
{
  public:
    constexpr Index()
      : mValue(0)
    {
    }

    explicit constexpr Index(T aValue)
      : mValue(aValue)
    {
    }

    /// @brief Get the underlying index value
    [[nodiscard]] constexpr T Get() const noexcept { return mValue; }

    /// @brief Add a Count to this Index, returning a new Index
    /// @param aOther The Count to add
    /// @return A new Index representing first-past-the-end position
    [[nodiscard]] constexpr Index<T, Tag> operator+(Count<T, Tag> aOther) const noexcept
    {
        return Index<T, Tag>(mValue + aOther.Get());
    }

    [[nodiscard]] constexpr bool operator>(Index<T, Tag> aOther) const noexcept
    {
        return mValue > aOther.Get();
    }

    [[nodiscard]] constexpr bool operator<(Index<T, Tag> aOther) const noexcept
    {
        return mValue < aOther.Get();
    }

    [[nodiscard]] constexpr bool operator==(Index<T, Tag> aOther) const noexcept
    {
        return mValue == aOther.Get();
    }

  private:
    T mValue;
};

// === Sample types (single channel values) ===

/// @brief Count of samples (always non-negative)
class SampleCount : public Count<size_t, TagSample>
{
  public:
    // inherit constructors
    using Count<size_t, TagSample>::Count;
};

/// @brief Index into audio timeline (0-based sample position)
class SampleIndex : public Index<size_t, TagSample>
{
  public:
    using Index<size_t, TagSample>::Index;

    /// @brief Add a SampleCount to this SampleIndex
    /// @param aOther The SampleCount to add
    /// @return A new SampleIndex offset by the given SampleCount
    /// @note this may be used to represent first-past-the-end for ranges
    [[nodiscard]] constexpr SampleIndex operator+(SampleCount aOther) const noexcept
    {
        return SampleIndex(Get() + aOther.Get());
    }

    /// @brief Cast to ptrdiff_t for use in iterator arithmetic
    /// @return The index as ptrdiff_t
    [[nodiscard]] constexpr std::ptrdiff_t AsPtrDiffT() const noexcept
    {
        return static_cast<std::ptrdiff_t>(Get());
    }
};

// === Frame types (multi-channel time positions) ===
// A frame represents one point in time across all channels.
// In mono: 1 frame = 1 sample. In stereo: 1 frame = 2 samples.

/// A signed frame position in the audio timeline, which can be negative to
/// represent positions before the timeline start
class FramePosition : public Index<int64_t, TagFrame>
{
  public:
    using Index<int64_t, TagFrame>::Index;

    /// @brief Add a FrameCount to this FramePosition
    /// @param aOther The FrameCount to add
    /// @return A new FramePosition offset by the given FrameCount
    [[nodiscard]] constexpr FramePosition operator+(FrameCount aOther) const noexcept;

    /// @brief Subtract a FrameCount from this FramePosition
    /// @param aOther The FrameCount to subtract
    /// @return A new FramePosition offset by the given FrameCount
    [[nodiscard]] constexpr FramePosition operator-(FrameCount aOther) const noexcept;

    /// @brief Add an FFTSize offset to this FramePosition
    /// @param aOther The FFTSize offset to add
    /// @return A new FramePosition offset by the given FFTSize
    /// @note this may be used to represent first-past-the-end for ranges
    [[nodiscard]] constexpr FramePosition operator+(FFTSize aOther) const noexcept
    {
        return FramePosition{ Get() + aOther };
    }

    /// @brief Subtract a FFTSize from this FramePosition
    /// @param aOther The FFTSize to subtract
    /// @return A new FramePosition offset by the given FFTSize
    [[nodiscard]] constexpr FramePosition operator-(FFTSize aOther) const noexcept
    {
        return FramePosition{ Get() - aOther };
    }
};

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
        return SampleCount(Get() * aChannels);
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

    [[nodiscard]] constexpr FramePosition AsPosition() const noexcept
    {
        return FramePosition{ ToIntChecked() };
    }
};

/// Index into audio timeline (0-based frame position)
class FrameIndex : public Index<size_t, TagFrame>
{
  public:
    using Index<size_t, TagFrame>::Index;
};

[[nodiscard]] constexpr FramePosition
FramePosition::operator+(FrameCount aOther) const noexcept
{
    return FramePosition{ Get() + static_cast<int64_t>(aOther.Get()) };
}

[[nodiscard]] constexpr FramePosition
FramePosition::operator-(FrameCount aOther) const noexcept
{
    return FramePosition{ Get() - static_cast<int64_t>(aOther.Get()) };
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
