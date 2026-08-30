#!/usr/bin/env sh
# Generate reproducible manufacturing/review files for one KiCad board.
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BOARD_DIR=${1:-$ROOT/hardware/rp2350_zero_carrier}
OUT=${2:-$ROOT/build/release}
BOARD=$(basename "$BOARD_DIR")
PCB=$BOARD_DIR/$BOARD.kicad_pcb
SCH=$BOARD_DIR/$BOARD.kicad_sch
SYM=$BOARD_DIR/$BOARD.kicad_sym
SCHEMATIC_GOLDEN=$BOARD_DIR/schematic_visual_golden.sha256
JLC_BOM=$BOARD_DIR/jlc_bom.csv
JLC_CPL=$BOARD_DIR/jlc_cpl.csv
DECISION=$BOARD_DIR/ORDER_DECISION_JA.md
WAIVERS=$BOARD_DIR/VALIDATION_WAIVERS_JA.md
GERBERS=$OUT/gerbers
PDF_WORK=$(mktemp -d)
trap 'rm -rf "$PDF_WORK"' EXIT

case "$(kicad-cli --version)" in
  10.0.5) ;;
  *) echo "KiCad CLI 10.0.5 is required; found: $(kicad-cli --version)" >&2; exit 1 ;;
esac
test -f "$PCB"
test -f "$SCH"
test -f "$SYM"
test -s "$SCHEMATIC_GOLDEN"
test -s "$JLC_BOM"
test -s "$JLC_CPL"
test -s "$DECISION"
test -s "$WAIVERS"
rm -rf "$OUT"
mkdir -p "$GERBERS"

# Keep the reviewed JLCPCB assembly files beside the generated fabrication
# outputs so the CI artifact and GitHub Release contain one coherent hardware
# package. These CSVs are maintained with the board source rather than derived
# by KiCad, so copy them byte-for-byte and include them in the checksums below.
cp "$JLC_BOM" "$OUT/jlc_bom.csv"
cp "$JLC_CPL" "$OUT/jlc_cpl.csv"
cp "$DECISION" "$OUT/ORDER_DECISION_JA.md"
cp "$WAIVERS" "$OUT/VALIDATION_WAIVERS_JA.md"

# Gate the exact pad/net/BOM/CPL contract independently of KiCad's schematic
# comparison, and retain the command output as manufacturing evidence.
python3 "$ROOT/scripts/hardware_contract.py" \
  > "$OUT/${BOARD}-hardware-contract.txt"

# KiCad reports annotation failures as warnings while still returning success.
# Export both native deliverables, then fail closed on '?' references,
# nonstandard references, missing components, and project-local symbol drift.
NATIVE_NETLIST=$OUT/${BOARD}-native.net
NATIVE_BOM=$OUT/${BOARD}-native-bom.csv
kicad-cli sch export netlist --format kicadsexpr \
  --output "$NATIVE_NETLIST" "$SCH"
kicad-cli sch export bom --output "$NATIVE_BOM" "$SCH"
test -s "$NATIVE_NETLIST"
test -s "$NATIVE_BOM"
python3 "$ROOT/scripts/schematic_contract.py" \
  "$SCH" "$SYM" "$NATIVE_NETLIST" "$NATIVE_BOM" \
  >> "$OUT/${BOARD}-hardware-contract.txt"

# Electrical errors are fatal.  Keep the complete warning report as evidence:
# standard-library lookup warnings and the intentional PCB-only UART endpoints
# remain real, classified warnings and are neither hidden nor called errors.
kicad-cli sch erc --severity-error --exit-code-violations \
  --output "$OUT/${BOARD}-erc-errors.rpt" "$SCH"
kicad-cli sch erc --severity-all \
  --output "$OUT/${BOARD}-erc-all.rpt" "$SCH"

# Keep DRC, unrouted items, and native schematic/PCB parity violations fatal so
# a hardware artifact cannot silently ship with a mismatched net or footprint.
kicad-cli pcb drc --schematic-parity --refill-zones --severity-all \
  --all-track-errors --output "$OUT/${BOARD}-drc.rpt" \
  --exit-code-violations "$PCB"
python3 "$ROOT/scripts/silk_audit.py" "$PCB"
kicad-cli pcb export gerbers --output "$GERBERS" \
  --layers F.Cu,B.Cu,F.Paste,B.Paste,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts "$PCB"
kicad-cli pcb export drill --output "$GERBERS" --format excellon \
  --generate-report --report-path "$GERBERS/drill-report.rpt" "$PCB"
# The source schematic is reviewed directly in A4 landscape. Export it without
# rescaling so property placement and the golden render describe the same page.
PDF_SCH_RAW=$PDF_WORK/${BOARD}-schematic-raw.pdf
kicad-cli sch export pdf --output "$PDF_SCH_RAW" --no-background-color "$SCH"
command -v gs >/dev/null || { echo "Ghostscript (gs) is required for A4 PDF output" >&2; exit 1; }
cp "$PDF_SCH_RAW" "$OUT/${BOARD}-schematic.pdf"
# KiCad's PCB exporter uses the board coordinate origin as the page origin. The
# carrier has negative coordinates, so exporting the source board clips it at
# the upper-left corner. Move a temporary copy in KiCad internal units, then
# export each selected layer on its own page at scale 1.0. This keeps the PDF
# vector and preserves the board's physical 1:1 size.
PDF_PCB_RAW=$PDF_WORK/${BOARD}-layout-raw.pdf
PCB_PDF_INPUT=$PDF_WORK/${BOARD}-layout-input.kicad_pcb
python3 - "$PCB" "$PCB_PDF_INPUT" <<'PY'
import pcbnew
import sys

source, output = sys.argv[1:]
board = pcbnew.LoadBoard(source)
# Board extents are approximately -33..23.5 x -41..18.5 mm.  This offset
# places the 56.5 x 59.5 mm board at the center of A4 while retaining scale 1.
delta = pcbnew.VECTOR2I(153000000, 116000000)
for footprint in board.GetFootprints():
    footprint.Move(delta)
tracks = board.Tracks()
for index in range(tracks.size()):
    tracks[index].Move(delta)
drawings = board.Drawings()
for index in range(len(drawings)):
    drawings[index].Move(delta)
for zone in board.Zones():
    zone.Move(delta)
pcbnew.SaveBoard(output, board)
PY
kicad-cli pcb export pdf --output "$PDF_PCB_RAW" --mode-multipage --scale 1 \
  --layers F.Cu,B.Cu,F.Silkscreen,B.Silkscreen,Edge.Cuts "$PCB_PDF_INPUT"
cp "$PDF_PCB_RAW" "$OUT/${BOARD}-layout.pdf"
for pdf in "$OUT/${BOARD}-schematic.pdf" "$OUT/${BOARD}-layout.pdf"; do
  test -s "$pdf"
  head -c 5 "$pdf" | grep -qx '%PDF-'
  tail -c 1024 "$pdf" | grep -aq '%%EOF'
done

# Check every page's A4 MediaBox and rendered-content bounds so paper-setting
# regressions or page-edge clipping cannot silently reach a release artifact.
python3 - "$OUT/${BOARD}-schematic.pdf" "$OUT/${BOARD}-layout.pdf" <<'PY'
from pathlib import Path
import re
import subprocess
import sys

expected = (841.896, 595.296)  # ISO A4 landscape, points
expected_page_counts = (1, 5)  # schematic; one page per selected PCB layer
for name, expected_page_count in zip(sys.argv[1:], expected_page_counts):
    data = Path(name).read_bytes()
    page_count = len(re.findall(rb"/Type\s*/Page\b", data))
    if page_count != expected_page_count:
        raise SystemExit(
            f"unexpected PDF page count: {name}: {page_count} "
            f"(expected {expected_page_count})"
        )
    media_boxes = re.findall(
        rb"/MediaBox\s*\[\s*0\s+0\s+([0-9.]+)\s+([0-9.]+)\s*\]", data
    )
    if not media_boxes:
        raise SystemExit(f"missing PDF MediaBox: {name}")
    for page, media_box in enumerate(media_boxes, 1):
        size = tuple(float(value) for value in media_box)
        if any(abs(actual - wanted) > 0.1 for actual, wanted in zip(size, expected)):
            raise SystemExit(f"PDF page is not A4 landscape: {name}: box {page}: {size}")
    bbox = subprocess.run(
        ["gs", "-q", "-dBATCH", "-dNOPAUSE", "-sDEVICE=bbox", name],
        check=True, capture_output=True,
    )
    content_boxes = re.findall(
        rb"%%HiResBoundingBox:\s*([-0-9.]+)\s+([-0-9.]+)\s+"
        rb"([-0-9.]+)\s+([-0-9.]+)", bbox.stderr,
    )
    if len(content_boxes) != page_count:
        raise SystemExit(
            f"missing PDF content bounds: {name}: {len(content_boxes)} "
            f"(expected {page_count})"
        )
    margin = 0.5  # points; content at the media edge is probably clipped
    for page, content_box in enumerate(content_boxes, 1):
        left, bottom, right, top = (float(value) for value in content_box)
        if (left < margin or bottom < margin or
                right > expected[0] - margin or top > expected[1] - margin):
            raise SystemExit(
                f"PDF content reaches outside A4 safe bounds: "
                f"{name}: page {page}: {(left, bottom, right, top)}"
            )
PY

python3 "$ROOT/scripts/schematic_visual_audit.py" \
  "$SCH" "$OUT/${BOARD}-schematic.pdf" "$SCHEMATIC_GOLDEN" \
  >> "$OUT/${BOARD}-hardware-contract.txt"

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

(cd "$OUT" && find . -maxdepth 1 -type f ! -name SHA256SUMS.txt \
  -printf '%f\0' | sort -z | xargs -0 sha256sum) > "$OUT/SHA256SUMS.txt"
(cd "$OUT" && sha256sum --check SHA256SUMS.txt)
echo "== release hardware artifacts: $BOARD"
find "$OUT" -maxdepth 1 -type f -printf '%f\n' | sort
