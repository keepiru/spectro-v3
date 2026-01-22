# Spectro-v3

Real-time spectrum analyzer with waterfall spectrogram and scrollable history buffer.

### Installing on Debian/Ubuntu

```bash
sudo apt update
sudo apt install \
    qt6-base-dev \
    qt6-multimedia-dev \
    libfftw3-dev \
    libcatch2-dev \
    cmake \
    build-essential \
    ninja-build \
    libsndfile1-dev \
    mold
```

## Build Instructions

```bash
# Build
make

# Run tests
make test

# Run linter on all source files
make lint

# Run linter with automatic fixes (use with caution)
make lint-fix
```

## Project Structure

```
spectro-v3/
├── dsp/              # DSP library (pure C++, no Qt dependencies)
│   ├── include/      # Public DSP headers
│   ├── src/          # DSP implementation files
│   └── tests/        # DSP unit tests
├── qt6_gui/          # Qt6-based GUI application (MVC architecture)
│   ├── controllers/  # Controllers
│   ├── models/       # Models
│   ├── views/        # Views
│   ├── include/      # GUI utility headers
│   ├── src/          # Main entry point and MainWindow
│   └── tests/        # GUI component unit tests
├── docs/             # Architecture and design documentation
└── build/            # Build artifacts (not tracked)
```

## License

Spectro-v3 is licensed under the [GNU General Public License v3.0](LICENSE) (GPL-3.0-only).

Copyright (C) 2025-2026 Chris "Kai" Frederick

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3 of the License only.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
