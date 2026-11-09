#!/usr/bin/env sh
# Package the committed Frisk (RP2350A + CH345T) fabrication set as release
# assets. The board has no KiCad schematic yet, so fab/ is the reviewed source
# of truth. This script verifies the expected file set and repackages it under
# release names. The ZIP uses fixed timestamps so repeated runs of the same
# commit produce the same checksum.
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BOARD=rp2350_frisk
BOARD_DIR=$ROOT/hardware/$BOARD
FAB=$BOARD_DIR/fab
GERBERS=$FAB/gerbers
DOCS=$BOARD_DIR/docs
OUT=${1:-$ROOT/build/release}
ZIP_NAME=${BOARD}_ch345_gerbers_JLCPCB.zip

for file in "$FAB/drc.rpt" "$FAB/jlc_bom.csv" "$FAB/jlc_cpl.csv" \
            "$FAB/bom_full.csv" "$FAB/ORDER_NOTES_JA.md" \
            "$DOCS/placement.pdf"; do
  test -s "$file" || { echo "missing fabrication input: $file" >&2; exit 1; }
done

mkdir -p "$OUT"
rm -f "$OUT/$ZIP_NAME" "$OUT/$BOARD-drc.rpt" "$OUT/$BOARD-jlc-bom.csv" \
      "$OUT/$BOARD-jlc-cpl.csv" "$OUT/$BOARD-bom-full.csv" \
      "$OUT/$BOARD-ORDER_NOTES_JA.md" "$OUT/$BOARD-placement.pdf"

python3 - "$GERBERS" "$OUT/$ZIP_NAME" "$BOARD" <<'PY'
from pathlib import Path
import sys
import zipfile

source = Path(sys.argv[1])
archive = Path(sys.argv[2])
board = sys.argv[3]
layers = (
    ("F_Cu", "gtl"), ("B_Cu", "gbl"),
    ("In1_Cu", "g1"), ("In2_Cu", "g2"),
    ("In3_Cu", "g3"), ("In4_Cu", "g4"),
    ("F_Paste", "gtp"), ("B_Paste", "gbp"),
    ("F_Silkscreen", "gto"), ("B_Silkscreen", "gbo"),
    ("F_Mask", "gts"), ("B_Mask", "gbs"),
    ("Edge_Cuts", "gm1"),
)
expected = {f"{board}-{layer}.{ext}" for layer, ext in layers}
expected |= {f"{board}.drl", f"{board}-job.gbrjob"}

actual = {path.name for path in source.iterdir() if path.is_file()}
if actual != expected:
    raise SystemExit(
        "unexpected Gerber set: "
        f"missing={sorted(expected - actual)} extra={sorted(actual - expected)}"
    )

with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as output:
    for name in sorted(expected):
        info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
        info.compress_type = zipfile.ZIP_DEFLATED
        output.writestr(info, (source / name).read_bytes())

with zipfile.ZipFile(archive) as contents:
    if set(contents.namelist()) != expected:
        raise SystemExit("manufacturing archive contents changed during write")
PY

cp "$FAB/drc.rpt" "$OUT/$BOARD-drc.rpt"
cp "$FAB/jlc_bom.csv" "$OUT/$BOARD-jlc-bom.csv"
cp "$FAB/jlc_cpl.csv" "$OUT/$BOARD-jlc-cpl.csv"
cp "$FAB/bom_full.csv" "$OUT/$BOARD-bom-full.csv"
cp "$FAB/ORDER_NOTES_JA.md" "$OUT/$BOARD-ORDER_NOTES_JA.md"
cp "$DOCS/placement.pdf" "$OUT/$BOARD-placement.pdf"

echo "== Frisk fabrication assets: $OUT"
find "$OUT" -maxdepth 1 -type f -printf '%f\n' | sort
