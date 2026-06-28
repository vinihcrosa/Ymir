# Stage 1: builder — system deps only, no source
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc-12 \
    g++-12 \
    cmake \
    ninja-build \
    git \
    curl \
    ca-certificates \
    libcgal-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 100

WORKDIR /src
# Stage 2: ci — copies source, builds, and tests
FROM builder AS ci

COPY core/ /src/core/

RUN cmake -B /build -S /src/core \
        -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DYMIR_BUILD_TESTS=ON \
    && cmake --build /build --parallel \
    && ctest --test-dir /build --output-on-failure
