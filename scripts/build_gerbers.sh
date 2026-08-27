#!/usr/bin/env sh
# Generate manufacturing files for the RP2350-Zero carrier.
#
# The output directory is deliberately ignored by Git: release CI uploads it as
# an artifact and attaches the archive, DRC report, and checksums to a release.
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
PCB="$ROOT/hardware/rp2350_zero_carrier/rp2350_zero_carrier.kicad_pcb"
OUT="${1:-$ROOT/build/release}"
GERBERS="$OUT/gerbers"
BOARD_NAME="rp2350_zero_carrier"
ZIP="$OUT/${BOARD_NAME}_gerbers_JLCPCB.zip"
DRC="$OUT/${BOARD_NAME}-drc.rpt"
SUMS="$OUT/SHA256SUMS.txt"

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

# KiCad writes the .gbrjob alongside the Gerbers.  Archive every generated
# Gerber/job file plus the Excellon drill file, but not diagnostic reports.
(
    cd "$GERBERS"
    zip -q "$ZIP" "${BOARD_NAME}"*.g* "${BOARD_NAME}".drl
)

# Catch accidental KiCad configuration changes that would produce an unusable
# manufacturing archive before publishing an artifact.
for file in \
    "${BOARD_NAME}-F_Cu.gtl" \
    "${BOARD_NAME}-B_Cu.gbl" \
    "${BOARD_NAME}-F_Paste.gtp" \
    "${BOARD_NAME}-B_Paste.gbp" \
    "${BOARD_NAME}-F_Silkscreen.gto" \
    "${BOARD_NAME}-B_Silkscreen.gbo" \
    "${BOARD_NAME}-F_Mask.gts" \
    "${BOARD_NAME}-B_Mask.gbs" \
    "${BOARD_NAME}-Edge_Cuts.gm1" \
    "${BOARD_NAME}.drl" \
    "${BOARD_NAME}-job.gbrjob"
do
    unzip -Z1 "$ZIP" | grep -qx "$file"
done

(cd "$OUT" && sha256sum \
    "$(basename "$ZIP")" "$(basename "$DRC")") > "$SUMS"

echo "== release hardware artifacts"
ls -l "$ZIP" "$DRC" "$SUMS"
