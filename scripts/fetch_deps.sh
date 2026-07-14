#!/usr/bin/env sh
# ---------------------------------------------------------------------------
#  ビルドに必要な外部ライブラリを lib/ 以下へ取得する。
#    - upstream hathach/tinyusb (MIDI ホスト対応版)  … SDK 同梱版は非対応
#    - sekigon-gonnoc/Pico-PIO-USB (RP2350 対応は main)
#  再現性のためタグ / コミットを固定している。
# ---------------------------------------------------------------------------
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LIB="$ROOT/lib"
mkdir -p "$LIB"

# --- バージョン固定 ---
TINYUSB_TAG="0.20.0"                                   # idx ベース tuh_midi API を含む最初のタグ
PIO_USB_REF="main"                                     # RP2350 対応は main 系。必要なら下でコミット固定
# PIO_USB_COMMIT=""                                    # 例: 検証済みコミットに固定したい場合はここへ

clone() {
    url="$1"; dst="$2"; ref="$3"
    if [ -d "$dst/.git" ]; then
        echo "== already present: $dst (skip)"
        return
    fi
    echo "== cloning $url @ $ref"
    git clone --depth 1 --branch "$ref" "$url" "$dst"
}

clone "https://github.com/hathach/tinyusb.git"           "$LIB/tinyusb"       "$TINYUSB_TAG"
clone "https://github.com/sekigon-gonnoc/Pico-PIO-USB.git" "$LIB/Pico-PIO-USB" "$PIO_USB_REF"

# 任意: Pico-PIO-USB を検証済みコミットに固定する
if [ -n "${PIO_USB_COMMIT:-}" ]; then
    echo "== pinning Pico-PIO-USB to $PIO_USB_COMMIT"
    ( cd "$LIB/Pico-PIO-USB" && git fetch --depth 1 origin "$PIO_USB_COMMIT" && git checkout "$PIO_USB_COMMIT" )
fi

echo "done. libs are in $LIB"
echo "  tinyusb      : $TINYUSB_TAG"
echo "  Pico-PIO-USB : $PIO_USB_REF"
