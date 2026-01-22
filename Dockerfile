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
    libfftw3-single3 \
    clang-format \
    libqt6test6 \
    libsndfile1-dev \
    ninja-build \
    sudo

# Add new requirements here so we don't have to rebuild the whole image
RUN eatmydata apt-get -yq install \
    gdb \
    mold

# Create user with matching UID/GID
RUN groupadd -g ${GROUP_ID} builder && \
    useradd -m -u ${USER_ID} -g builder -s /bin/bash builder && \
    echo "builder ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER builder
WORKDIR /build
ENV QT_QPA_PLATFORM=offscreen

# For now, just provide a shell
CMD ["/bin/bash"]
