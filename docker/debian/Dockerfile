# Spectro-v3 -- Real-time spectrum analyzer
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2025-2026 Chris "Kai" Frederick

# Dockerfile for spectro-v3 - Real-time spectrum analyzer
# Based on Debian Trixie (testing)

FROM debian:trixie AS builder

ARG USER_ID=1000
ARG GROUP_ID=1000

# Install build dependencies
RUN apt-get update && \
    apt-get install -yq eatmydata && \
    eatmydata apt-get install -yq \
    build-essential \
    clang-format \
    clang-tidy-19 \
    cmake \
    g++-14 \
    gdb \
    git \
    lcov \
    libcatch2-dev \
    libfftw3-dev \
    libfftw3-single3 \
    libqt6core6 \
    libqt6gui6 \
    libqt6multimedia6 \
    libqt6test6 \
    libqt6widgets6 \
    libsndfile1-dev \
    mold \
    ninja-build \
    pkg-config \
    qt6-base-dev \
    qt6-multimedia-dev \
    sudo

# Add new requirements here so we don't have to rebuild the whole image
# RUN eatmydata apt-get -yq install ...

# Create user with matching UID/GID
RUN groupadd -g ${GROUP_ID} builder && \
    useradd -m -u ${USER_ID} -g builder -s /bin/bash builder && \
    echo "builder ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER builder
WORKDIR /build
ENV QT_QPA_PLATFORM=offscreen
ENV CC=/usr/bin/gcc-14
ENV CXX=/usr/bin/g++-14

# For now, just provide a shell
CMD ["/bin/bash"]
