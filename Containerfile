FROM docker.io/devkitpro/devkitarm:20260221 AS toolchain

ENV DEVKITPRO=/opt/devkitpro
ENV DEVKITARM=/opt/devkitpro/devkitARM
ENV PATH=/opt/devkitpro/tools/bin:/opt/devkitpro/devkitARM/bin:${PATH}

RUN set -eu; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
      clang-format \
      cmake \
      ninja-build \
      ssh \
      libopus-dev; \
    rm -rf /var/lib/apt/lists/*

RUN set -eu; \
    git clone https://github.com/Epicpkmn11/bannertool /tmp/bannertool; \
    cd /tmp/bannertool; \
    git checkout 51dac927a4c43de769900d91fd3cb6142295263f; \
    git clone https://github.com/UselessMnemonic/buildtools-ext buildtools; \
    cd buildtools; \
    git checkout fd424efb1d95f1a4d755881943d1e6ccd5d31d48; \
    cd ..; \
    make install; \
    rm -rf /tmp/bannertool

RUN set -eu; \
    git clone https://github.com/3DSGuy/Project_CTR /tmp/Project_CTR; \
    cd /tmp/Project_CTR; \
    git checkout c0488dcb6c3048e6a519716f2b21e2c65859ca75; \
    cd makerom; \
    make deps program; \
    cp bin/makerom /usr/local/bin/makerom; \
    rm -rf /tmp/Project_CTR


FROM toolchain AS dev

WORKDIR /workspace/lunar3d


FROM toolchain AS ci

COPY . /workspace/lunar3d
WORKDIR /workspace/lunar3d
RUN cmake --preset devkitarm
RUN cmake --build --preset 3dsx
RUN cmake --build --preset cia
