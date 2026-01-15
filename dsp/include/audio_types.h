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

/// @brief Mixin to add a value of type Other to Self, returning Self
/// @tparam Other The Count type to add
/// @note The typical usage is to add a quantity (Count) to an absolute position
/// (Index/Position), returning a new absolute position.
template<typename Other>
class Add
{
  protected:
    constexpr Add() = default;

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

template<typename Other>
class Subtract
{
  public:
    /// @brief Subtract a Count from this value
    /// @param aOther The Count to subtract
    /// @return A new Self value offset by the given Count
    template<typename Self>
    [[nodiscard]] constexpr Self operator-(this const Self& aSelf, const Other& aOther) noexcept
    {
        return Self(aSelf.Get() - aOther.Get());
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

/// @brief FFT transform size (must be a power of 2)
///
/// This is a special case of Count used in FFT operations.
/// The constructor validates it fits in int for FFTW3 compatibility.
class FFTSize
  : public audio_type_internal::Base<size_t>
  , public audio_type_internal::Comparable
{
  public:
    /// @brief Construct an FFT size value
    /// @param aValue The FFT size (must be a positive power of 2 that fits in int)
    /// @throws std::invalid_argument if aValue is not a positive power of 2 or exceeds int max
    constexpr FFTSize(size_t aValue)
      : audio_type_internal::Base<size_t>(aValue)
    {
        // Ensure aValue fits in int for FFTW3 compatibility
        if (aValue > static_cast<size_t>(std::numeric_limits<int>::max())) {
            throw std::invalid_argument(std::format(
              "FFTSize({}) exceeds int max({})", aValue, std::numeric_limits<int>::max()));
        }
        // Ensure aValue is a positive power of 2
        if (aValue == 0 || (aValue & (aValue - 1)) != 0) {
            throw std::invalid_argument("FFTSize must be a positive power of 2");
        }
    }

    /// @brief Implicit access as int for convenience
    /// @return The FFT size as an int (safe because constructor validates it fits)
    /// @note FFTSize is used in many contexts with varying semantics: as a
    /// parameter for FFTW, as a size for buffers, as a count of samples, etc.
    /// Calling .Get() everywhere undermines the purpose of having a strong type
    /// in the first place. We might want to revisit this if we add more
    /// semantic operations here later.
    [[nodiscard]] constexpr operator int() const noexcept { return static_cast<int>(Get()); }
};

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
  , public audio_type_internal::Add<SampleCount>
  , public audio_type_internal::Comparable
  , public audio_type_internal::AsPtrDiff
{
  public:
    using audio_type_internal::Base<size_t>::Base;
};

/// @brief A signed frame position in the audio timeline
///
/// Can be negative to represent positions before the timeline start.
class FramePosition
  : public audio_type_internal::Base<std::ptrdiff_t>
  , public audio_type_internal::Add<FrameCount>
  , public audio_type_internal::Add<FFTSize>
  , public audio_type_internal::Subtract<FrameCount>
  , public audio_type_internal::Subtract<FFTSize>
  , public audio_type_internal::Comparable
{
  public:
    using audio_type_internal::Base<std::ptrdiff_t>::Base;
    using audio_type_internal::Add<FrameCount>::operator+;
    using audio_type_internal::Add<FFTSize>::operator+;
    using audio_type_internal::Subtract<FrameCount>::operator-;
    using audio_type_internal::Subtract<FFTSize>::operator-;
};

/// @brief Count of frames (always non-negative)
class FrameCount
  : public audio_type_internal::Base<size_t>
  , public audio_type_internal::Comparable
  , public audio_type_internal::AsPtrDiff
  , public audio_type_internal::Add<FrameCount>
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

    /// @brief Convert to FramePosition
    /// @return The FramePosition equivalent of this FrameCount
    /// @throws std::overflow_error if the count exceeds std::ptrdiff_t maximum
    [[nodiscard]] constexpr FramePosition AsPosition() const
    {
        const size_t kValue = Get();
        if (kValue > std::numeric_limits<std::ptrdiff_t>::max()) {
            throw std::overflow_error(
              std::format("FrameCount({}) exceeds std::ptrdiff_t max", kValue));
        }
        return FramePosition{ static_cast<std::ptrdiff_t>(kValue) };
    }
};

/// @brief Index into audio timeline (0-based frame position)
class FrameIndex
  : public audio_type_internal::Base<size_t>
  , public audio_type_internal::Add<FrameCount>
  , public audio_type_internal::Comparable
{
  public:
    using audio_type_internal::Base<size_t>::Base;
};
