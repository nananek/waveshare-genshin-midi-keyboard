#!/usr/bin/env sh
# Generate manufacturing files for the RP2350-Zero carrier.
#
# The output directory is deliberately ignored by Git: release CI uploads it as
# an artifact and attaches the archive, DRC report, and checksums to a release.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
PCB="$ROOT/hardware/rp2350_zero_carrier/rp2350_zero_carrier.kicad_pcb"
SCH="$ROOT/hardware/rp2350_zero_carrier/rp2350_zero_carrier.kicad_sch"
OUT="${1:-$ROOT/build/release}"
GERBERS="$OUT/gerbers"
BOARD_NAME="rp2350_zero_carrier"
ZIP="$OUT/${BOARD_NAME}_gerbers_JLCPCB.zip"
DRC="$OUT/${BOARD_NAME}-drc.rpt"
SUMS="$OUT/SHA256SUMS.txt"
SCHEMATIC_PDF="$OUT/${BOARD_NAME}-schematic.pdf"
PCB_PDF="$OUT/${BOARD_NAME}-layout.pdf"

case "$(kicad-cli --version)" in
    10.0.*) ;;
    *)
        echo "KiCad CLI 10.0.x is required; found: $(kicad-cli --version)" >&2
        exit 1
        ;;
esac

rm -rf "$OUT"
mkdir -p "$GERBERS"

# --exit-code-violations makes errors, warnings, and unconnected pads fail the
# build.  The report is retained as a release asset for manufacturing review.
kicad-cli pcb drc --output "$DRC" --exit-code-violations "$PCB"
# Export only the layers a board fabricator needs.  The board's plot settings
# also enable documentation layers (Fab, Courtyard, User.*), which do not
# belong in a manufacturing archive.
kicad-cli pcb export gerbers --output "$GERBERS" \
    --layers F.Cu,B.Cu,F.Paste,B.Paste,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts \
    "$PCB"
kicad-cli pcb export drill --output "$GERBERS" --format excellon \
    --generate-report --report-path "$GERBERS/drill-report.rpt" "$PCB"

# Include the requested schematic and board-layout drawings as review assets.
kicad-cli sch export pdf --output "$SCHEMATIC_PDF" --no-background-color "$SCH"
kicad-cli pcb export pdf --output "$PCB_PDF" --mode-multipage \
    --layers F.Cu,B.Cu,F.Silkscreen,B.Silkscreen,Edge.Cuts "$PCB"

# The pinned KiCad image does not include a PDF parser.  Check the required
# PDF header and trailer so a truncated or wrong-format drawing cannot ship.
for pdf in "$SCHEMATIC_PDF" "$PCB_PDF"
do
    test -s "$pdf"
    head -c 5 "$pdf" | grep -qx '%PDF-'
    tail -c 1024 "$pdf" | grep -aq '%%EOF'
done

# KiCad writes the .gbrjob alongside the Gerbers.  Archive every generated
# Gerber/job file plus the Excellon drill file, but not diagnostic reports.
# The pinned KiCad image has Python but not zip/unzip; use Python's standard
# library to keep the image reference reproducible without extra packages.
python3 - "$ZIP" "$GERBERS" "$BOARD_NAME" <<'PY'
from pathlib import Path
import sys
import zipfile

archive, directory = map(Path, sys.argv[1:3])
board_name = sys.argv[3]
files = sorted(directory.glob(f"{board_name}*.g*")) + [directory / f"{board_name}.drl"]
if not all(path.is_file() for path in files):
    raise SystemExit("missing Gerber, job, or Excellon drill output")
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as output:
    for path in files:
        output.write(path, path.name)
PY

# Catch accidental KiCad configuration changes that would produce an unusable
# manufacturing archive before publishing an artifact.
python3 - "$ZIP" "$BOARD_NAME" <<'PY'
import sys
import zipfile

archive, board_name = sys.argv[1:]
expected = {
    f"{board_name}-F_Cu.gtl", f"{board_name}-B_Cu.gbl",
    f"{board_name}-F_Paste.gtp", f"{board_name}-B_Paste.gbp",
    f"{board_name}-F_Silkscreen.gto", f"{board_name}-B_Silkscreen.gbo",
    f"{board_name}-F_Mask.gts", f"{board_name}-B_Mask.gbs",
    f"{board_name}-Edge_Cuts.gm1", f"{board_name}.drl",
    f"{board_name}-job.gbrjob",
}
with zipfile.ZipFile(archive) as contents:
    actual = set(contents.namelist())
if actual != expected:
    raise SystemExit(f"unexpected manufacturing archive contents: {sorted(actual)}")
PY

(cd "$OUT" && sha256sum \
    "$(basename "$ZIP")" "$(basename "$DRC")" \
    "$(basename "$SCHEMATIC_PDF")" "$(basename "$PCB_PDF")") > "$SUMS"

echo "== release hardware artifacts"
ls -l "$ZIP" "$DRC" "$SCHEMATIC_PDF" "$PCB_PDF" "$SUMS"
