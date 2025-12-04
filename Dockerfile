# Dockerfile for spectro-v3 - Real-time spectrum analyzer
# Based on Debian Trixie (testing)

FROM debian:trixie AS builder

# Install build dependencies
RUN apt-get update && apt-get install -yq \
    build-essential \
    cmake \
    pkg-config \
    qt6-base-dev \
    qt6-multimedia-dev \
    libfftw3-dev \
    libcatch2-dev \
    clang-tidy \
    libqt6core6 \
    libqt6gui6 \
    libqt6widgets6 \
    libqt6multimedia6 \
    libfftw3-single3

# Add new requirements here so we don't have to rebuild the whole image
RUN apt-get -yq install \
    clang-format \
    libqt6test6

# Set working directory
WORKDIR /build

# For now, just provide a shell
CMD ["/bin/bash"]
