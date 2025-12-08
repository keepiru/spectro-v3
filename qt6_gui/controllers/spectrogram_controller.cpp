#include "spectrogram_controller.h"
#include "../models/audio_buffer.h"
#include <fft_window.h>
#include <ifft_processor.h>
#include <sample_buffer.h>
#include <stdexcept>
#include <stft_processor.h>

SpectrogramController::SpectrogramController(AudioBuffer& aAudioBuffer,
                                             FFTProcessorFactory aFFTProcessorFactory,
                                             FFTWindowFactory aFFTWindowFactory,
                                             QObject* aParent)
  : QObject(aParent)
  , mAudioBuffer(aAudioBuffer)
  , mFFTProcessorFactory(std::move(aFFTProcessorFactory))
  , mFFTWindowFactory(std::move(aFFTWindowFactory))
  , mWindowStride(0)
{
}
