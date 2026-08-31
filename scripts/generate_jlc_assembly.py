#!/usr/bin/env python3
"""Generate the reviewed JLC BOM/CPL from KiCad's native position export.

The PCB is the source of placement truth.  The small JSON manifest supplies
the reviewed purchasing identity and the one documented JLC rotation offset
(U1); it deliberately contains no coordinates or side information.
"""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile


BOM_HEADER = ("Comment", "Designator", "Footprint", "LCSC Part #")
CPL_HEADER = ("Designator", "Mid X", "Mid Y", "Layer", "Rotation")
NATIVE_HEADER = ("Ref", "Val", "Package", "PosX", "PosY", "Rot", "Side")
MANIFEST_NAME = "jlc_assembly.json"

# These are reviewed purchasing identities, not suggestions for a matcher to
# replace. Keep the complete set here so changing the manifest cannot silently
# turn a shortage into an unreviewed substitute. The manifest remains the
# readable source for the generated files, while this table is the fail-closed
# approval gate for the current board revision.
REVIEWED_COMPONENTS: dict[str, dict[str, object]] = {
    "U1": {
        "reference": "U1", "pcb_value": "TPS2553DBVR",
        "bom_comment": "TPS2553DBVR", "footprint": "TPS2553DBVR",
        "manufacturer": "Texas Instruments", "mpn": "TPS2553DBVR",
        "lcsc": "C55266", "jlc_rotation_offset": 180,
    },
    "C1": {
        "reference": "C1", "pcb_value": "1uF X7R 16V",
        "bom_comment": "1uF X7R 16V", "footprint": "C_0603",
        "manufacturer": "FH (Guangdong Fenghua Advanced Tech)",
        "mpn": "0603B105K160NT", "lcsc": "C93816",
        "jlc_rotation_offset": 0,
    },
    "R3": {
        "reference": "R3", "pcb_value": "52.3k 1%",
        "bom_comment": "52.3k 1%", "footprint": "R_0603",
        "manufacturer": "UNI-ROYAL (Uniroyal Elec)",
        "mpn": "0603WAF5232T5E", "lcsc": "C23198",
        "jlc_rotation_offset": 0,
    },
    "R4": {
        "reference": "R4", "pcb_value": "100k",
        "bom_comment": "100k 1%", "footprint": "R_0603",
        "manufacturer": "YAGEO", "mpn": "RC0603FR-07100KL",
        "lcsc": "C14675", "jlc_rotation_offset": 0,
    },
    "R5": {
        "reference": "R5", "pcb_value": "100k EN pulldown",
        "bom_comment": "100k 1%", "footprint": "R_0603",
        "manufacturer": "YAGEO", "mpn": "RC0603FR-07100KL",
        "lcsc": "C14675", "jlc_rotation_offset": 0,
    },
    "C3": {
        "reference": "C3", "pcb_value": "100nF X7R 50V",
        "bom_comment": "100nF X7R 50V", "footprint": "C_0603",
        "manufacturer": "YAGEO", "mpn": "CC0603KRX7R9BB104",
        "lcsc": "C14663", "jlc_rotation_offset": 0,
    },
}


class AssemblyError(RuntimeError):
    """Raised for any drift that must stop a manufacturing build."""


@dataclass(frozen=True)
class NativePosition:
    reference: str
    value: str
    footprint: str
    x: float
    y: float
    rotation: float
    side: str


def fail(message: str) -> None:
    raise AssemblyError(message)


def normalized_rotation(value: float) -> float:
    result = value % 360.0
    return 0.0 if result == 0.0 else result


def parse_number(value: str, context: str) -> float:
    try:
        return float(value)
    except ValueError as error:
        raise AssemblyError(f"non-numeric {context}: {value!r}") from error


def read_csv_rows(path: Path, expected_header: tuple[str, ...]) -> list[dict[str, str]]:
    try:
        with path.open(newline="", encoding="utf-8-sig") as source:
            reader = csv.reader(source)
            rows = list(reader)
    except OSError as error:
        raise AssemblyError(f"cannot read {path}: {error}") from error
    if not rows:
        fail(f"empty CSV: {path}")
    if tuple(rows[0]) != expected_header:
        fail(f"unexpected header in {path}: {rows[0]!r}")
    result = []
    for line_number, row in enumerate(rows[1:], 2):
        if len(row) != len(expected_header):
            fail(f"wrong column count in {path}:{line_number}: {len(row)}")
        if not any(row):
            fail(f"empty record in {path}:{line_number}")
        result.append(dict(zip(expected_header, row, strict=True)))
    return result


def load_manifest(board_dir: Path) -> list[dict[str, object]]:
    path = board_dir / MANIFEST_NAME
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise AssemblyError(f"cannot read manifest {path}: {error}") from error
    if data.get("schema_version") != 1 or not isinstance(data.get("components"), list):
        fail(f"unsupported manifest schema: {path}")
    required = {
        "reference", "pcb_value", "bom_comment", "footprint",
        "manufacturer", "mpn", "lcsc", "jlc_rotation_offset",
    }
    components: list[dict[str, object]] = []
    references: set[str] = set()
    for index, component in enumerate(data["components"]):
        if not isinstance(component, dict) or set(component) != required:
            fail(f"invalid manifest component {index}: expected {sorted(required)}")
        if not all(isinstance(component[key], str) and component[key]
                   for key in required - {"jlc_rotation_offset"}):
            fail(f"invalid manifest strings for component {index}")
        if type(component["jlc_rotation_offset"]) not in (int, float):
            fail(f"invalid JLC rotation offset for component {index}")
        reference = str(component["reference"])
        if reference in references:
            fail(f"duplicate manifest reference: {reference}")
        references.add(reference)
        components.append(component)
    if references != set(REVIEWED_COMPONENTS):
        fail(f"unexpected assembly manifest references: {sorted(references)}")
    manifest_by_reference = {
        str(component["reference"]): component for component in components
    }
    for reference, expected in REVIEWED_COMPONENTS.items():
        if manifest_by_reference[reference] != expected:
            fail(f"unreviewed assembly identity or rotation for {reference}")
    return components


def parse_native_rows(path: Path) -> dict[str, NativePosition]:
    rows = read_csv_rows(path, NATIVE_HEADER)
    result: dict[str, NativePosition] = {}
    for row in rows:
        reference = row["Ref"].strip()
        if not reference or reference in result:
            fail(f"empty or duplicate native reference in {path}: {reference!r}")
        side = row["Side"].strip().lower()
        if side not in {"top", "bottom"}:
            fail(f"invalid native side for {reference}: {row['Side']!r}")
        result[reference] = NativePosition(
            reference=reference,
            value=row["Val"],
            footprint=row["Package"],
            x=parse_number(row["PosX"], f"native X for {reference}"),
            y=parse_number(row["PosY"], f"native Y for {reference}"),
            rotation=normalized_rotation(
                parse_number(row["Rot"], f"native rotation for {reference}")),
            side=side,
        )
    return result


def export_native_positions(board_dir: Path, destination: Path | None = None
                            ) -> dict[str, NativePosition]:
    board = board_dir / f"{board_dir.name}.kicad_pcb"
    if not board.is_file():
        fail(f"missing PCB: {board}")
    temporary: tempfile.NamedTemporaryFile[str] | None = None
    if destination is None:
        temporary = tempfile.NamedTemporaryFile(prefix="jlc-native-pos-", suffix=".csv",
                                                delete=False)
        destination = Path(temporary.name)
        temporary.close()
    destination.parent.mkdir(parents=True, exist_ok=True)
    command = [
        "kicad-cli", "pcb", "export", "pos", "--format", "csv", "--units", "mm",
        "--side", "both", "--smd-only", "--exclude-dnp", "--output", str(destination),
        str(board),
    ]
    try:
        subprocess.run(command, check=True, text=True, capture_output=True)
        return parse_native_rows(destination)
    except (OSError, subprocess.CalledProcessError) as error:
        detail = error.stderr.strip() if isinstance(error, subprocess.CalledProcessError) else str(error)
        raise AssemblyError(f"native KiCad position export failed: {detail}") from error
    finally:
        if temporary is not None:
            destination.unlink(missing_ok=True)


def material_key(component: dict[str, object]) -> tuple[str, str, str, str, str]:
    return tuple(str(component[key]) for key in (
        "bom_comment", "footprint", "manufacturer", "mpn", "lcsc"))


def checked_native_positions(components: list[dict[str, object]],
                             native: dict[str, NativePosition]
                             ) -> dict[str, NativePosition]:
    expected = {str(component["reference"]) for component in components}
    if set(native) != expected:
        fail(f"native SMD assembly set: expected {sorted(expected)}, got {sorted(native)}")
    for component in components:
        reference = str(component["reference"])
        position = native[reference]
        if position.value != component["pcb_value"]:
            fail(f"{reference} native PCB value: {position.value!r}")
        if position.footprint != component["footprint"]:
            fail(f"{reference} native PCB footprint: {position.footprint!r}")
    return native


def csv_bytes(header: tuple[str, ...], rows: list[dict[str, str]]) -> bytes:
    output = io.StringIO(newline="")
    writer = csv.DictWriter(output, fieldnames=header, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return output.getvalue().encode("utf-8")


def render_assembly(components: list[dict[str, object]],
                    native: dict[str, NativePosition]) -> tuple[bytes, bytes]:
    checked_native_positions(components, native)
    grouped: dict[tuple[str, str, str, str, str], list[dict[str, object]]] = {}
    for component in components:
        grouped.setdefault(material_key(component), []).append(component)
    bom_rows = []
    for group in grouped.values():
        first = group[0]
        bom_rows.append({
            "Comment": str(first["bom_comment"]),
            "Designator": ",".join(str(component["reference"]) for component in group),
            "Footprint": str(first["footprint"]),
            "LCSC Part #": str(first["lcsc"]),
        })
    cpl_rows = []
    for component in sorted(components, key=lambda item: str(item["reference"])):
        reference = str(component["reference"])
        position = native[reference]
        cpl_rows.append({
            "Designator": reference,
            "Mid X": f"{position.x:.6f}",
            "Mid Y": f"{position.y:.6f}",
            "Layer": "Top" if position.side == "top" else "Bottom",
            "Rotation": f"{normalized_rotation(position.rotation + float(component['jlc_rotation_offset'])):.6f}",
        })
    return csv_bytes(BOM_HEADER, bom_rows), csv_bytes(CPL_HEADER, cpl_rows)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("board_dir", type=Path, nargs="?",
                        default=Path("hardware/rp2350_zero_carrier"))
    parser.add_argument("--check", action="store_true",
                        help="fail unless committed BOM/CPL match the native render")
    parser.add_argument("--native-pos", type=Path,
                        help="read a saved native position CSV (test-only)")
    parser.add_argument("--native-pos-output", type=Path,
                        help="retain the native KiCad CSV used for this render")
    args = parser.parse_args()

    try:
        components = load_manifest(args.board_dir)
        if args.native_pos is not None:
            native = parse_native_rows(args.native_pos)
            if args.native_pos_output is not None:
                args.native_pos_output.parent.mkdir(parents=True, exist_ok=True)
                args.native_pos_output.write_bytes(args.native_pos.read_bytes())
        else:
            native = export_native_positions(args.board_dir, args.native_pos_output)
        bom, cpl = render_assembly(components, native)
        bom_path = args.board_dir / "jlc_bom.csv"
        cpl_path = args.board_dir / "jlc_cpl.csv"
        if args.check:
            for path, expected in ((bom_path, bom), (cpl_path, cpl)):
                if not path.is_file() or path.read_bytes() != expected:
                    fail(f"{path} is not the deterministic native KiCad render; run this generator")
        else:
            bom_path.write_bytes(bom)
            cpl_path.write_bytes(cpl)
        print("JLC ASSEMBLY GENERATOR PASS")
        print("  native KiCad position source: C1,C3,R3,R4,R5,U1")
        print("  BOM material rows: 5; C14675 designators: R4,R5 (quantity 2)")
        print("  CPL placement rows: 6; U1 reviewed JLC rotation correction: +180")
    except AssemblyError as error:
        raise SystemExit(f"JLC ASSEMBLY GENERATOR FAIL: {error}") from error


if __name__ == "__main__":
    main()
