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
kicad-cli pcb export gerbers --output "$GERBERS" "$PCB"
kicad-cli pcb export drill --output "$GERBERS" --format excellon \
    --generate-report --report-path "$GERBERS/drill-report.rpt" "$PCB"

# Review PDFs are release assets too: one logical schematic sheet and one
# single-page layout containing both copper/silkscreen sides and the outline.
kicad-cli sch export pdf --output "$SCHEMATIC_PDF" --no-background-color "$SCH"
kicad-cli pcb export pdf --output "$PCB_PDF" --mode-single \
    --layers F.Cu,B.Cu,F.Silkscreen,B.Silkscreen,Edge.Cuts "$PCB"

# KiCad writes the .gbrjob alongside the Gerbers.  Archive every generated
# Gerber/job file plus the Excellon drill file, but not diagnostic reports.
(
    cd "$GERBERS"
    zip -q "$ZIP" "${BOARD_NAME}"*.g* "${BOARD_NAME}".drl
)

# Catch accidental KiCad configuration changes that would produce an unusable
# manufacturing archive before publishing an artifact.
unzip -Z1 "$ZIP" | grep -qx "${BOARD_NAME}-F_Cu.gtl"
unzip -Z1 "$ZIP" | grep -qx "${BOARD_NAME}-B_Cu.gbl"
unzip -Z1 "$ZIP" | grep -qx "${BOARD_NAME}-Edge_Cuts.gm1"
unzip -Z1 "$ZIP" | grep -qx "${BOARD_NAME}.drl"
unzip -Z1 "$ZIP" | grep -qx "${BOARD_NAME}-job.gbrjob"

(cd "$OUT" && sha256sum \
    "$(basename "$ZIP")" "$(basename "$DRC")" \
    "$(basename "$SCHEMATIC_PDF")" "$(basename "$PCB_PDF")") > "$SUMS"

echo "== release hardware artifacts"
ls -l "$ZIP" "$DRC" "$SCHEMATIC_PDF" "$PCB_PDF" "$SUMS"
