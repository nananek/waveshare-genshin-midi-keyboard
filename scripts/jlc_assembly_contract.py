#!/usr/bin/env python3
"""Validate JLC BOM/CPL semantics, independently of JLC's web matcher."""

from __future__ import annotations

import argparse
from collections import Counter
from pathlib import Path
import re

from generate_jlc_assembly import (
    AssemblyError,
    BOM_HEADER,
    CPL_HEADER,
    NativePosition,
    checked_native_positions,
    export_native_positions,
    load_manifest,
    normalized_rotation,
    parse_native_rows,
    read_csv_rows,
)


REFERENCE = re.compile(r"^[A-Z]+[1-9][0-9]*$")


def fail(message: str) -> None:
    raise AssemblyError(message)


def expanded_designators(row: dict[str, str], row_number: int) -> tuple[str, ...]:
    values = tuple(item.strip() for item in row["Designator"].split(","))
    if not values or any(not value or not REFERENCE.fullmatch(value) for value in values):
        fail(f"invalid BOM Designator at row {row_number}: {row['Designator']!r}")
    if len(values) != len(set(values)):
        fail(f"duplicate designator inside BOM row {row_number}: {row['Designator']!r}")
    return values


def expected_material_rows(components: list[dict[str, object]]) -> dict[
        tuple[str, str, str], set[str]]:
    result: dict[tuple[str, str, str], set[str]] = {}
    for component in components:
        key = (str(component["bom_comment"]), str(component["footprint"]),
               str(component["lcsc"]))
        result.setdefault(key, set()).add(str(component["reference"]))
    return result


def validate_jlc_assembly(board_dir: Path,
                          native: dict[str, NativePosition] | None = None) -> None:
    components = load_manifest(board_dir)
    expected_references = {str(component["reference"]) for component in components}
    if native is None:
        native = export_native_positions(board_dir)
    checked_native_positions(components, native)

    bom_rows = read_csv_rows(board_dir / "jlc_bom.csv", BOM_HEADER)
    cpl_rows = read_csv_rows(board_dir / "jlc_cpl.csv", CPL_HEADER)
    if len(bom_rows) != 5:
        fail(f"JLC BOM must have 5 material rows, got {len(bom_rows)}")
    if len(cpl_rows) != len(expected_references):
        fail(f"JLC CPL must have {len(expected_references)} placement rows, got {len(cpl_rows)}")

    actual_materials: dict[tuple[str, str, str], set[str]] = {}
    all_bom_refs: list[str] = []
    lcsc_rows: Counter[str] = Counter()
    for row_number, row in enumerate(bom_rows, 2):
        key = (row["Comment"], row["Footprint"], row["LCSC Part #"])
        if key in actual_materials:
            fail(f"duplicate JLC material row: {key}")
        refs = set(expanded_designators(row, row_number))
        actual_materials[key] = refs
        all_bom_refs.extend(refs)
        lcsc_rows[row["LCSC Part #"]] += 1
    duplicates = sorted(reference for reference, count in Counter(all_bom_refs).items()
                        if count != 1)
    if duplicates:
        fail(f"BOM designators must appear exactly once: {duplicates}")
    if set(all_bom_refs) != expected_references:
        fail(f"BOM expanded designators: expected {sorted(expected_references)}, "
             f"got {sorted(set(all_bom_refs))}")
    if actual_materials != expected_material_rows(components):
        fail(f"BOM material grouping is not the reviewed manifest: {actual_materials}")
    duplicated_lcsc = sorted(lcsc for lcsc, count in lcsc_rows.items() if count != 1)
    if duplicated_lcsc:
        fail(f"LCSC appears in more than one BOM material row: {duplicated_lcsc}")
    c14675 = next((refs for (comment, footprint, lcsc), refs in actual_materials.items()
                   if lcsc == "C14675"), None)
    if c14675 != {"R4", "R5"}:
        fail(f"C14675 must be exactly one material row for R4,R5, got {c14675}")

    cpl_by_ref: dict[str, dict[str, str]] = {}
    for row_number, row in enumerate(cpl_rows, 2):
        reference = row["Designator"].strip()
        if not REFERENCE.fullmatch(reference):
            fail(f"invalid CPL Designator at row {row_number}: {reference!r}")
        if reference in cpl_by_ref:
            fail(f"duplicate CPL designator: {reference}")
        if row["Layer"] not in {"Top", "Bottom"}:
            fail(f"invalid CPL layer for {reference}: {row['Layer']!r}")
        try:
            x, y, rotation = (float(row["Mid X"]), float(row["Mid Y"]),
                              float(row["Rotation"]))
        except ValueError as error:
            raise AssemblyError(f"non-numeric CPL placement for {reference}") from error
        if not 0.0 <= rotation < 360.0:
            fail(f"CPL rotation outside [0, 360) for {reference}: {rotation}")
        cpl_by_ref[reference] = row
    if set(cpl_by_ref) != expected_references:
        fail(f"CPL designators: expected {sorted(expected_references)}, "
             f"got {sorted(cpl_by_ref)}")

    component_by_ref = {str(component["reference"]): component for component in components}
    for reference, component in component_by_ref.items():
        row = cpl_by_ref[reference]
        position = native[reference]
        expected = (
            position.x, position.y,
            "Top" if position.side == "top" else "Bottom",
            normalized_rotation(position.rotation + float(component["jlc_rotation_offset"])),
        )
        actual = (float(row["Mid X"]), float(row["Mid Y"]), row["Layer"],
                  float(row["Rotation"]))
        if actual != expected:
            fail(f"{reference} CPL/native placement: expected {expected}, got {actual}")

    u1 = component_by_ref["U1"]
    if u1["mpn"] != "TPS2553DBVR" or u1["lcsc"] != "C55266":
        fail("U1 substitution or LCSC change")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("board_dir", type=Path, nargs="?",
                        default=Path("hardware/rp2350_zero_carrier"))
    parser.add_argument("--native-pos", type=Path,
                        help="saved native position CSV (used by tests)")
    args = parser.parse_args()
    try:
        native = parse_native_rows(args.native_pos) if args.native_pos else None
        validate_jlc_assembly(args.board_dir, native)
        print("JLC ASSEMBLY CONTRACT PASS")
        print("  BOM material rows=5; C14675=R4,R5; quantity per PCB=2")
        print("  CPL placement rows=6; R4=(18,12,Top,90), R5=(5.5,13.5,Top,0)")
        print("  native KiCad position parity; U1=C55266/TPS2553DBVR/Top/180")
    except AssemblyError as error:
        raise SystemExit(f"JLC ASSEMBLY CONTRACT FAIL: {error}") from error


if __name__ == "__main__":
    main()
