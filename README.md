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

