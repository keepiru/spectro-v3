# Settings panel implementation notes

Include comprehensive tests.

Make sure to name elements so you can reference them in tests.

Use lambdas for the callbacks instead of creating slots.

Come up with a sensible layout structure.

It should include these elements:

- Pulldown: "Window Type"
- Pulldown: "FFT Size" - 512, 1024, 2048, 4096, 8192
- Slider: "Window Scale"
- Slider: Aperture min - from -80 to +30 db
- Slider: Aperture max - from -80 to +30 db
- 6 pulldowns for colormaps:
  - Name
  - 128x16 Icon showing a sample of the gradient: setIconSize(QSize(128,16))
