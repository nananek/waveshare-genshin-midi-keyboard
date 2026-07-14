# RP2350 ファームをビルドするための ARM クロスビルド環境。
#   - arm-none-eabi ツールチェーン (Debian bookworm: 12.2、RP2350/Cortex-M33 対応)
#   - pico-sdk (RP2350 対応タグ)
#   - picotool (SDK 2.x の uf2/bin 生成に必要)
# tinyusb はプロジェクト側で upstream を指す (PICO_TINYUSB_PATH) ため submodule 不要。
FROM debian:bookworm-slim

ARG PICO_SDK_TAG=2.1.1
ARG PICOTOOL_TAG=2.1.1

RUN apt-get update && apt-get install -y --no-install-recommends \
        cmake \
        git \
        ca-certificates \
        python3 \
        build-essential \
        gcc-arm-none-eabi \
        binutils-arm-none-eabi \
        libnewlib-arm-none-eabi \
        libstdc++-arm-none-eabi-newlib \
        libusb-1.0-0-dev \
        pkg-config \
    && rm -rf /var/lib/apt/lists/*

ENV PICO_SDK_PATH=/opt/pico-sdk

# pico-sdk
RUN git clone --depth 1 --branch ${PICO_SDK_TAG} \
        https://github.com/raspberrypi/pico-sdk.git ${PICO_SDK_PATH}

# picotool (install to /usr/local so find_package(picotool) が見つける)
RUN git clone --depth 1 --branch ${PICOTOOL_TAG} \
        https://github.com/raspberrypi/picotool.git /tmp/picotool \
    && cmake -S /tmp/picotool -B /tmp/picotool/build -DCMAKE_INSTALL_PREFIX=/usr/local \
    && cmake --build /tmp/picotool/build -j "$(nproc)" \
    && cmake --install /tmp/picotool/build \
    && rm -rf /tmp/picotool

# 任意 uid での bind mount に対する git の dubious-ownership を回避
RUN git config --system --add safe.directory '*'

WORKDIR /work
