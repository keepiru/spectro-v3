#pragma once
#include <QObject>
#include <cstddef>
#include <memory>
#include <sample_buffer.h>
#include <vector>

/**
 * @brief Multi-channel audio buffer for real-time spectrum analyzer
 *
 * Wraps multiple SampleBuffer instances (one per channel) and provides
 * Qt signal/slot integration for the MVC architecture. Thread-safe for
 * concurrent writes from audio capture thread and reads from processing thread.
 *
 * Inherits QObject to emit signals when new audio data arrives.
 */
class AudioBuffer : public QObject
{
    Q_OBJECT

  public:
    /**
     * @brief Constructor
     * @param aChannelCount Number of audio channels (1=mono, 2=stereo, etc.)
     * @param aSampleRate Sample rate in Hz
     * @param aParent Qt parent object (optional)
     */
    explicit AudioBuffer(size_t aChannelCount, size_t aSampleRate, QObject* aParent = nullptr);

    /**
     * @brief Get the number of channels
     * @return Channel count
     */
    [[nodiscard]] size_t GetChannelCount() const { return mChannelCount; }

    /**
     * @brief Get the sample rate
     * @return Sample rate in Hz
     */
    [[nodiscard]] size_t GetSampleRate() const { return mSampleRate; }

    /**
     * @brief Get total number of samples in a specific channel
     * @param aChannelIndex Channel index (0-based)
     * @return Number of samples stored
     * @throws std::out_of_range if aChannelIndex >= channel count
     */
    [[nodiscard]] size_t GetNumSamples(size_t aChannelIndex) const;

    /**
     * @brief Add interleaved audio samples to all channels
     * @param aSamples Interleaved audio data (channel 0, channel 1, ..., repeat)
     * @throws std::invalid_argument if sample count not divisible by channel count
     *
     * Example for stereo (2 channels): [L0, R0, L1, R1, L2, R2, ...]
     * This method de-interleaves and appends to respective channel buffers.
     *
     * Emits dataAvailable() signal after samples are added.
     */
    void AddSamples(const std::vector<float>& aSamples);

    /**
     * @brief Get samples from a specific channel
     * @param aChannelIndex Channel index (0-based)
     * @param aStartSample Starting sample index (can be negative for zero-padding)
     * @param aSampleCount Number of samples to retrieve
     * @return Vector of samples (zero-padded if out of bounds)
     * @throws std::out_of_range if aChannelIndex >= channel count
     */
    [[nodiscard]] std::vector<float> GetSamples(size_t aChannelIndex,
                                                int64_t aStartSample,
                                                size_t aSampleCount) const;

    /**
     * @brief Get reference to underlying SampleBuffer for a channel
     * @param aChannelIndex Channel index (0-based)
     * @return Reference to SampleBuffer
     * @throws std::out_of_range if aChannelIndex >= channel count
     *
     * Useful for passing to STFTProcessor which expects SampleBuffer&.
     */
    [[nodiscard]] SampleBuffer& GetChannelBuffer(size_t aChannelIndex);
    [[nodiscard]] const SampleBuffer& GetChannelBuffer(size_t aChannelIndex) const;

  signals:
    /**
     * @brief Emitted when new audio samples are added
     * @param aSampleCount Number of samples added per channel
     */
    void dataAvailable(size_t aSampleCount);

  private:
    size_t mChannelCount;
    size_t mSampleRate;
    std::vector<std::unique_ptr<SampleBuffer>> mChannelBuffers;
};
