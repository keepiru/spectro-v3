# Settings panel implementation notes

```
+---------------------------------------------------------------+
|                   SETTINGS PANEL                              |
|                      (QVBoxLayout)                            |
+---------------------------------------------------------------+
|                                                               |
|  +-- Audio Source (QGroupBox) -----------------------------+  |
|  |                                                         |  |
|  |                  [ Open file ]                          |  |
|  |                                                         |  |
|  |  +-- QFormLayout --+----------------------------+       |  |
|  |  |  Audio Device:  | [v Default Input Device ]  |       |  |
|  |  |  Sample Rate:   | [v 44100 Hz             ]  |       |  |
|  |  |  Channels:      | [v 2                    ]  |       |  |
|  |  +-----------------+----------------------------+       |  |
|  |                                                         |  |
|  |                  [ Start Recording ]                    |  |
|  |                                                         |  |
|  +---------------------------------------------------------+  |
|                                                               |
|  +-- FFT (QGroupBox) --------------------------------------+  |
|  |  +-- QFormLayout ------+-----------------------------+  |  |
|  |  |  Window Type:       | [v Hann                  ]  |  |  |
|  |  |  FFT Size:          | [v 2048                  ]  |  |  |
|  |  |  Window Scale:      | [-----o-------------]  2    |  |  |
|  |  |  Aperture Floor:    | [----o--------------] -20   |  |  |
|  |  |  Aperture Ceiling:  | [-----------o-------]  40   |  |  |
|  |  +---------------------+-----------------------------+  |  |
|  +---------------------------------------------------------+  |
|                                                               |
|  +-- Color Map (QGroupBox) -------------------------------+   |
|  |  +-- QFormLayout +----------------------------------+  |   |
|  |  |  Color Map 1: | [v Viridis  ][################]  |  |   |
|  |  |  Color Map 2: | [v Plasma   ][################]  |  |   |
|  |  +---------------+----------------------------------+  |   |
|  +--------------------------------------------------------+   |
|                                                               |
|  +-- Display (QGroupBox) ---------------------------------+   |
|  |                                                        |   |
|  |           [ Live Mode ]                                |   |
|  |                                                        |   |
|  +--------------------------------------------------------+   |
|                                                               |
+---------------------------------------------------------------+
```

Include comprehensive tests.

Make sure to name elements so you can reference them in tests.

Use lambdas for the callbacks instead of creating slots.

Come up with a sensible layout structure.

It should include these elements:

## Audio Controls

Audio controls are disabled (grayed out) while recording is active.

- Pulldown: Audio Device
  - values from QMediaDevices::audioInputs()
- Pulldown: Sample rate
  - values from QAudioDevice::supportedSampleRates()
  - update when Audio Device changes
- Pulldown: Channels
  - populate from device
  - clamp to GKMaxChannels
- Button: Start/Stop Recording toggle

## FFT Controls

- Pulldown: "Window Type"
  - values from FFTWindow::Type
  - default Settings::GetWindowType()
- Pulldown: "FFT Size"
  - values from Settings::KValidFFTSizes
  - default Settings::GetFFTSize()
- Slider: "Window Scale"
  - range Settings::KValidWindowScales
  - default Settings::GetWindowScale()
- Slider: Aperture Floor
  - range Settings::KApertureLimitsDecibels
  - default Settings::GetApertureFloorDecibels
- Slider: Aperture Ceiling
  - range Settings::KApertureLimitsDecibels
  - default Settings::GetApertureCeilingDecibels

## Color Map Controls

- pulldowns for colormaps:
  - Name
  - 128x16 Icon showing a sample of the gradient: setIconSize(QSize(128,16))
    - generate gradient values by calling ColorMap::GeneratePreview()
  - values from ColorMap::Type
  - defaults from Settings::GetColorMapType()
- The number of pulldowns:
  - matches GetChannelCount()
  - may change on BufferReset()

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