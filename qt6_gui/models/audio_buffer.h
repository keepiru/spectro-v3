#pragma once
#include <QObject>
#include <cstddef>
#include <memory>
#include <sample_buffer.h>
#include <vector>

/**
 * @brief Multi-channel audio buffer
 *
 * Wraps multiple SampleBuffer instances (one per channel) and provides
 * Qt signal/slot integration for the MVC architecture.
 */
class AudioBuffer : public QObject
{
    Q_OBJECT

  public:
    /**
     * @brief Constructor
     * @param aParent Qt parent object (optional)
     */
    explicit AudioBuffer(QObject* aParent = nullptr);

    /**
     * @brief Reset the audio buffer, clearing all samples
     * @param aChannelCount New channel count
     * @param aSampleRate New sample rate in Hz
     * @throws std::invalid_argument if aChannelCount or aSampleRate is invalid
     * @note Intended to be used when starting recording or loading a file.
     */
    void Reset(size_t aChannelCount, size_t aSampleRate);

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
     * @param aStartSample Starting sample index
     * @param aSampleCount Number of samples to retrieve
     * @return Vector of samples.
     * @throws std::out_of_range if aChannelIndex >= channel count, or if there
     * aren't enough samples to fill the request.
     */
    [[nodiscard]] std::vector<float> GetSamples(size_t aChannelIndex,
                                                size_t aStartSample,
                                                size_t aSampleCount) const;

    /**
     * @brief Get the underlying SampleBuffer for a specific channel
     * @param aChannelIndex Channel index (0-based)
     * @return Reference to the SampleBuffer for the channel
     * @throws std::out_of_range if aChannelIndex >= channel count
     */
    [[nodiscard]] const SampleBuffer& GetChannelBuffer(size_t aChannelIndex) const;

    // Get the number of samples
    [[nodiscard]] int64_t NumSamples() const { return mChannelBuffers[0]->NumSamples(); }

  signals:
    /**
     * @brief Emitted when new audio samples are added
     * @param aSampleCount Number of samples added per channel
     */
    void DataAvailable(size_t aSampleCount);

    /**
     * @brief Emitted when the buffer is reset
     *
     * This notifies listeners to clear any cached data.
     */
    void BufferReset();

  private:
    /**
     * @brief Initialize empty mChannelBuffers with given channel count and sample rate
     * @param aChannelCount Number of audio channels
     * @param aSampleRate Sample rate in Hz
     * @throws std::invalid_argument if aChannelCount or aSampleRate is invalid
     * @note This is a helper function used by the constructor and Reset() method.
     */
    void InitializeChannelBuffers(size_t aChannelCount, size_t aSampleRate);

    size_t mChannelCount;
    size_t mSampleRate;
    std::vector<std::unique_ptr<SampleBuffer>> mChannelBuffers;
};
