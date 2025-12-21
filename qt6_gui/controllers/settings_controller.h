#pragma once

#include "models/settings.h"
#include <QAudioDevice>
#include <QObject>

/**
 * @brief Controller for Settings
 */
class SettingsController : public QObject
{
    Q_OBJECT
  public:
    /**
     * @brief Constructor
     * @param aSettings Pointer to Settings model
     * @param aParent Parent QObject
     */
    explicit SettingsController(Settings* aSettings, QObject* aParent = nullptr)
      : QObject(aParent)
      , mSettings(aSettings)
    {
    }

    /**
     * @brief Start audio recording with specified settings
     * @param aDevice Audio device to use for recording
     * @param aSampleRate Sample rate in Hz (e.g., 44100, 48000)
     * @param aChannelCount Number of audio channels (e.g., 1 for mono, 2 for stereo)
     * @throws std::invalid_argument if the sample rate or channel count is invalid
     *
     * Emits StartAudioRecording signal to notify AudioRecorder to start recording.
     */
    void StartRecording(const QAudioDevice& aDevice, int aSampleRate, int aChannelCount);

  signals:

    /**
     * @brief Notify AudioRecorder to start audio recording with current settings
     * @param aDevice Audio device to use for recording
     * @param aChannelCount Number of audio channels
     * @param aSampleRate Sample rate in Hz
     */
    void StartAudioRecording(const QAudioDevice& aDevice, int aChannelCount, int aSampleRate);

  private:
    Settings* mSettings;
};