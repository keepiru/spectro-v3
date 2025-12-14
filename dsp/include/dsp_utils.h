#pragma once
#include <cmath>

class DSPUtils
{
  public:
    /**
     * @brief Convert magnitude to decibels
     * @param aMagnitude Input magnitude value (must be > 0)
     * @return Decibel value (20 * log10(aMagnitude))
     *
     * Returns a large negative value (-1000.0f) for zero or negative input
     * to avoid undefined behavior.
     */
    [[nodiscard]] inline static float MagnitudeToDecibels(float aMagnitude) noexcept
    {
        // Handle zero and negative values to avoid log of non-positive numbers
        if (aMagnitude <= 0.0F) {
            constexpr float kMinimumDecibelValue = -1000.0F;
            return kMinimumDecibelValue; // Return a very small dB value
        }

        // Standard conversion: dB = 20 * log10(magnitude)
        constexpr float kDecibelScaleFactor = 20.0F;
        return kDecibelScaleFactor * std::log10(aMagnitude);
    }
};