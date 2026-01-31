# Settings panel implementation notes

Include comprehensive tests.

Make sure to name elements so you can reference them in tests.

Use lambdas for the callbacks instead of creating slots.

Come up with a sensible layout structure.

It should include these elements:

## Audio Controls

Audio controls are disabled (grayed out) while recording is active.

| Control      | Widget        | Description                                                           |
|--------------|---------------|-----------------------------------------------------------------------|
| Audio Device | `QComboBox`   | Select audio input device                                             |
| Sample Rate  | `QComboBox`   | Select sample rate (populated based on device capabilities)           |
| Channels     | `QSpinBox`    | Number of channels (range limited by device capabilities, max 6)      |
| Recording    | `QPushButton` | Start/Stop recording toggle                                           |

## FFT Controls

- Pulldown: "Window Type"
- Pulldown: "FFT Size" - values from Settings::KValidFFTSizes
- Slider: "Window Scale"
- Slider: Aperture Floor - from Settings::KApertureLimitsDecibels
- Slider: Aperture Ceiling - from Settings::KApertureLimtisDecibels

## Color Map Controls

- 6 pulldowns for colormaps:
  - Name
  - 128x16 Icon showing a sample of the gradient: setIconSize(QSize(128,16))

## Open File button

QPushButton("Open file");

When clicked:
- Choose file with QFileDialog
- Create a QProgressDialog
- Call AudioFile::LoadFile with the filename and progress callback lambda

## Live Mode button

QPushButton("Live Mode");

When clicked:
 - Settings::SetLiveMode(true);