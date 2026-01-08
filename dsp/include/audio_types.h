#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <sndfile.h>
#include <stdexcept>

// Forward declarations
class FrameCount;

/// Number of audio channels (e.g., 1=mono, 2=stereo)
using ChannelCount = uint8_t;

/// FFT transform size (must be a power of 2)
/// int is used for compatibility with FFTW3.
using FFTSize = int;

/// Audio sample rate in Hz (e.g., 44100, 48000)
/// int is used for compatibility with FFTW3 and libsndfile.
using SampleRate = int;

// Overview of the strong types defined in this file:
//
// Each type is in the form: [Sample|Frame][Count|Index|Position]
//
// Sample vs Frame:
//   Sample types refer to single-channel values (e.g., left channel only).
//   Frame types refer to multi-channel values (e.g., left + right channels).
//   In mono audio, 1 frame = 1 sample. In stereo audio, 1 frame = 2 samples.
//
// Count vs Index vs Position:
//   Count: represents a quantity/size (how many items).
//   Index: represents a zero-based position in the timeline.
//   Position: represents an abstract signed position in the timeline (can be negative).

/// @brief Namespace for strong type mixin classes
namespace audio_type_internal {

/// @brief Base class for strong types wrapping an underlying value
/// @tparam T The underlying value type
template<typename T>
class Base
{
  public:
    explicit constexpr Base(T aValue = 0)
      : mValue(aValue)
    {
    }

    /// @brief Get the underlying index value
    /// @return The stored value
    template<typename Self>
    [[nodiscard]] constexpr T Get(this const Self& aSelf) noexcept
    {
        return aSelf.mValue;
    }

  private:
    T mValue;
};

/// @brief Mixin to provide addition of Count types to Index/Position types
/// @tparam Other The Count type to add
template<typename Other>
class AddCount
{
  public:
    /// @brief Add a Count to this value
    /// @param aOther The Count to add
    /// @return A new Self value offset by the given Count
    template<typename Self>
    [[nodiscard]] constexpr Self operator+(this const Self& aSelf, const Other& aOther) noexcept
    {
        return Self(aSelf.Get() + aOther.Get());
    }
};

/// @brief Mixin to provide equality and three-way comparison operators
class Comparable
{
  public:
    /// @brief Equality comparison
    /// @param aOther The value to compare against
    /// @return true if the values are equal, false otherwise
    template<typename Self>
    [[nodiscard]] constexpr bool operator==(this const Self& aSelf, const Self& aOther) noexcept
    {
        return aSelf.Get() == aOther.Get();
    }

    /// @brief Three-way comparison
    /// @param aOther The value to compare against
    /// @return The comparison result
    template<typename Self>
    [[nodiscard]] constexpr auto operator<=>(this const Self& aSelf, const Self& aOther) noexcept
    {
        return aSelf.Get() <=> aOther.Get();
    }
};

/// @brief Mixin to allow casting to ptrdiff_t for iterator arithmetic
class AsPtrDiff
{
  public:
    /// @brief Cast to ptrdiff_t for use in iterator arithmetic
    /// @return The value as ptrdiff_t
    template<typename Self>
    [[nodiscard]] constexpr std::ptrdiff_t AsPtrDiffT(this const Self& aSelf) noexcept
    {
        return static_cast<std::ptrdiff_t>(aSelf.Get());
    }
};

} // namespace audio_type_internal

// === Sample types (single channel values) ===

/// @brief Count of samples (always non-negative)
class SampleCount
  : public audio_type_internal::Base<size_t>
  , public audio_type_internal::Comparable
  , public audio_type_internal::AsPtrDiff
{
  public:
    using audio_type_internal::Base<size_t>::Base;
};

/// @brief Index into audio timeline (0-based sample position)
class SampleIndex
  : public audio_type_internal::Base<size_t>
  , public audio_type_internal::AddCount<SampleCount>
  , public audio_type_internal::Comparable
  , public audio_type_internal::AsPtrDiff
{
  public:
    using audio_type_internal::Base<size_t>::Base;
};

/// A signed frame position in the audio timeline, which can be negative to
/// represent positions before the timeline start
class FramePosition
  : public audio_type_internal::Base<int64_t>
  , public audio_type_internal::AddCount<FrameCount>
  , public audio_type_internal::Comparable
{
  public:
    using audio_type_internal::Base<int64_t>::Base;
    using audio_type_internal::AddCount<FrameCount>::operator+;

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
class FrameCount
  : public audio_type_internal::Base<size_t>
  , public audio_type_internal::Comparable
  , public audio_type_internal::AsPtrDiff
{
  public:
    using audio_type_internal::Base<size_t>::Base;

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
class FrameIndex
  : public audio_type_internal::Base<size_t>
  , public audio_type_internal::AddCount<FrameCount>
  , public audio_type_internal::Comparable
{
  public:
    using audio_type_internal::Base<size_t>::Base;
};

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
