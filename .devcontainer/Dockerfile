FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        gcc \
        meson \
        ninja-build \
        pkg-config \
        libcurl4-gnutls-dev \
        libflac-dev \
        libexpat1-dev \
        libgtk-3-dev \
        libvte-2.91-dev \
        git \
        gdb \
        valgrind \
    && rm -rf /var/lib/apt/lists/*
