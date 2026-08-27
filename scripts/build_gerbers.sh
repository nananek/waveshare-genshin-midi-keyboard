#!/usr/bin/env sh
# Generate reproducible manufacturing/review files for one KiCad board.
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BOARD_DIR=${1:-$ROOT/hardware/rp2350_zero_carrier}
OUT=${2:-$ROOT/build/release/$(basename "$BOARD_DIR")}
BOARD=$(basename "$BOARD_DIR")
PCB=$BOARD_DIR/$BOARD.kicad_pcb
SCH=$BOARD_DIR/$BOARD.kicad_sch
GERBERS=$OUT/gerbers

case "$(kicad-cli --version)" in
  10.0.*) ;;
  *) echo "KiCad CLI 10.0.x is required; found: $(kicad-cli --version)" >&2; exit 1 ;;
esac
test -f "$PCB"
test -f "$SCH"
rm -rf "$OUT"
mkdir -p "$GERBERS"

# Keep DRC violations fatal so a hardware artifact cannot silently ship with
# shorts, missing connections, or manufacturing-rule errors.
kicad-cli pcb drc --output "$OUT/${BOARD}-drc.rpt" --exit-code-violations "$PCB"
kicad-cli pcb export gerbers --output "$GERBERS" \
  --layers F.Cu,B.Cu,F.Paste,B.Paste,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts "$PCB"
kicad-cli pcb export drill --output "$GERBERS" --format excellon \
  --generate-report --report-path "$GERBERS/drill-report.rpt" "$PCB"
kicad-cli sch export pdf --output "$OUT/${BOARD}-schematic.pdf" --no-background-color "$SCH"
kicad-cli pcb export pdf --output "$OUT/${BOARD}-layout.pdf" --mode-single \
  --layers F.Cu,B.Cu,F.Silkscreen,B.Silkscreen,Edge.Cuts "$PCB"

for pdf in "$OUT/${BOARD}-schematic.pdf" "$OUT/${BOARD}-layout.pdf"; do
  test -s "$pdf"
  head -c 5 "$pdf" | grep -qx '%PDF-'
  tail -c 1024 "$pdf" | grep -aq '%%EOF'
done

python3 - "$OUT/${BOARD}_gerbers_JLCPCB.zip" "$GERBERS" "$BOARD" <<'PY'
from pathlib import Path
import sys, zipfile
archive, directory, board = map(Path, sys.argv[1:])
files = sorted(directory.glob(f"{board.name}*.g*")) + [directory / f"{board.name}.drl"]
if not all(path.is_file() for path in files):
    raise SystemExit("missing Gerber, job, or Excellon drill output")
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as output:
    for path in files:
        output.write(path, path.name)

expected = {
    f"{board.name}-F_Cu.gtl", f"{board.name}-B_Cu.gbl",
    f"{board.name}-F_Paste.gtp", f"{board.name}-B_Paste.gbp",
    f"{board.name}-F_Silkscreen.gto", f"{board.name}-B_Silkscreen.gbo",
    f"{board.name}-F_Mask.gts", f"{board.name}-B_Mask.gbs",
    f"{board.name}-Edge_Cuts.gm1", f"{board.name}.drl",
    f"{board.name}-job.gbrjob",
}
with zipfile.ZipFile(archive) as contents:
    actual = set(contents.namelist())
if actual != expected:
    raise SystemExit(f"unexpected manufacturing archive contents: {sorted(actual)}")
PY

(cd "$OUT" && sha256sum \
  "${BOARD}_gerbers_JLCPCB.zip" "${BOARD}-drc.rpt" \
  "${BOARD}-schematic.pdf" "${BOARD}-layout.pdf") > "$OUT/SHA256SUMS.txt"
echo "== release hardware artifacts: $BOARD"
ls -l "$OUT/${BOARD}_gerbers_JLCPCB.zip" "$OUT/${BOARD}-drc.rpt" \
  "$OUT/${BOARD}-schematic.pdf" "$OUT/${BOARD}-layout.pdf" "$OUT/SHA256SUMS.txt"
