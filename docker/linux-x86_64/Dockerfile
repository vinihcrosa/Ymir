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

# SUNDIALS 7.7.0 from source (Ubuntu 22.04 ships 5.8.0, project requires >= 6.0)
ARG SUNDIALS_VERSION=7.7.0
RUN curl -fsSL "https://github.com/LLNL/sundials/releases/download/v${SUNDIALS_VERSION}/sundials-${SUNDIALS_VERSION}.tar.gz" \
        | tar xz -C /tmp \
    && cmake -B /tmp/sundials-build -S /tmp/sundials-${SUNDIALS_VERSION} \
        -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_TESTING=OFF \
        -DEXAMPLES_INSTALL=OFF \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
    && cmake --build /tmp/sundials-build --parallel \
    && cmake --install /tmp/sundials-build \
    && rm -rf /tmp/sundials-${SUNDIALS_VERSION} /tmp/sundials-build

WORKDIR /src

# Stage 2: ci — copies source, configures, builds, and tests
FROM builder AS ci

COPY . /src

RUN cmake -B /build -S /src \
        -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DYMIR_BUILD_TESTS=ON \
    && cmake --build /build --parallel \
    && ctest --test-dir /build --output-on-failure
