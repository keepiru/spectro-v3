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
     * @param aStartSample Starting sample index, possibly negative
     * @param aSampleCount Number of samples to retrieve
     * @return Vector of samples.  Invalid samples are filled with zeros.
     * @throws std::out_of_range if aChannelIndex >= channel count
     */
    [[nodiscard]] const std::vector<float> GetSamples(const size_t aChannelIndex,
                                                      const int64_t aStartSample,
                                                      const size_t aSampleCount) const;

    /**
     * @brief Get the underlying SampleBuffer for a specific channel
     * @param aChannelIndex Channel index (0-based)
     * @return Reference to the SampleBuffer for the channel
     * @throws std::out_of_range if aChannelIndex >= channel count
     */
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
