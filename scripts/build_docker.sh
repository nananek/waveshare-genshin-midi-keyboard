#!/usr/bin/env sh
# ---------------------------------------------------------------------------
#  Docker で ARM ファーム (build/genshin_midi_kbd.uf2) をビルドする。
#  ホストに ARM ツールチェーン / pico-sdk が無くても動く。
#
#    ./scripts/build_docker.sh            # イメージ構築 + ビルド
#    IMAGE=my/img ./scripts/build_docker.sh
#
#  依存ライブラリ (lib/tinyusb, lib/Pico-PIO-USB) はコンテナ内で fetch_deps.sh
#  が取得する (bind mount 先のホスト lib/ に書かれる。gitignore 済み)。
# ---------------------------------------------------------------------------
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
IMAGE="${IMAGE:-genshin-pico-builder}"

echo "== building image: $IMAGE"
docker build -t "$IMAGE" -f "$ROOT/docker/Dockerfile" "$ROOT/docker"

echo "== building firmware"
# rootless docker では container-root が呼び出しユーザにマップされるので --user を
# 付けてはいけない (bind mount に書けなくなる)。rootful docker では逆に --user で
# ホストユーザに合わせ、root 所有の成果物が生成されるのを防ぐ。
USER_ARGS=""
if ! docker info 2>/dev/null | grep -qi rootless; then
    USER_ARGS="--user $(id -u):$(id -g) -e HOME=/tmp"
fi

# shellcheck disable=SC2086
docker run --rm \
    -v "$ROOT:/work" -w /work \
    $USER_ARGS \
    "$IMAGE" \
    sh -c './scripts/fetch_deps.sh && cmake -B build -DPICO_BOARD=pico2 && cmake --build build -j "$(nproc)"'

echo "== artifacts"
ls -la "$ROOT/build"/genshin_midi_kbd.uf2 "$ROOT/build"/genshin_midi_kbd.elf 2>/dev/null \
    || { echo "  (uf2 が生成されていない — 上のログを確認)"; exit 1; }
