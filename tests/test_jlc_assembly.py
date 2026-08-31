#!/usr/bin/env python3
"""Negative tests for the JLC grouped-BOM / per-placement-CPL contract."""

from __future__ import annotations

import csv
import io
from pathlib import Path
import shutil
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from generate_jlc_assembly import AssemblyError, NativePosition  # noqa: E402
from jlc_assembly_contract import validate_jlc_assembly  # noqa: E402


NATIVE = {
    "C1": NativePosition("C1", "1uF X7R 16V", "C_0603", 8.5, 21.0, 180.0, "top"),
    "C3": NativePosition("C3", "100nF X7R 50V", "C_0603", 15.25, 20.8, 90.0, "top"),
    "R3": NativePosition("R3", "52.3k 1%", "R_0603", 14.5, 17.8, 180.0, "top"),
    "R4": NativePosition("R4", "100k", "R_0603", 18.0, 12.0, 90.0, "top"),
    "R5": NativePosition("R5", "100k EN pulldown", "R_0603", 5.5, 13.5, 0.0, "top"),
    "U1": NativePosition("U1", "TPS2553DBVR", "TPS2553DBVR", 11.0, 17.3, 0.0, "top"),
}


def read_rows(path: Path) -> list[list[str]]:
    with path.open(newline="", encoding="utf-8") as source:
        return list(csv.reader(source))


def write_rows(path: Path, rows: list[list[str]]) -> None:
    output = io.StringIO(newline="")
    csv.writer(output, lineterminator="\n").writerows(rows)
    path.write_text(output.getvalue(), encoding="utf-8")


class JlcAssemblyContractTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.board = Path(self.temporary.name)
        source = ROOT / "hardware" / "rp2350_zero_carrier"
        for name in ("jlc_assembly.json", "jlc_bom.csv", "jlc_cpl.csv"):
            shutil.copy2(source / name, self.board / name)

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def validate(self) -> None:
        validate_jlc_assembly(self.board, NATIVE)

    def assert_rejected(self) -> None:
        with self.assertRaises(AssemblyError):
            self.validate()

    def test_generated_files_pass(self) -> None:
        self.validate()

    def test_split_c14675_material_rows_fail(self) -> None:
        rows = read_rows(self.board / "jlc_bom.csv")
        row = next(row for row in rows if row and row[-1] == "C14675")
        rows.remove(row)
        rows.extend([
            ["100k 1%", "R4", "R_0603", "C14675"],
            ["100k 1%", "R5", "R_0603", "C14675"],
        ])
        write_rows(self.board / "jlc_bom.csv", rows)
        self.assert_rejected()

    def test_missing_cpl_placement_fails(self) -> None:
        rows = [row for row in read_rows(self.board / "jlc_cpl.csv")
                if not row or row[0] != "R5"]
        write_rows(self.board / "jlc_cpl.csv", rows)
        self.assert_rejected()

    def test_duplicate_bom_or_cpl_designator_fails(self) -> None:
        rows = read_rows(self.board / "jlc_bom.csv")
        next(row for row in rows if row and row[-1] == "C14675")[1] = "R4,R4"
        write_rows(self.board / "jlc_bom.csv", rows)
        self.assert_rejected()

        shutil.copy2(ROOT / "hardware" / "rp2350_zero_carrier" / "jlc_bom.csv",
                     self.board / "jlc_bom.csv")
        rows = read_rows(self.board / "jlc_cpl.csv")
        rows.append(["R4", "18.000000", "12.000000", "Top", "90.000000"])
        write_rows(self.board / "jlc_cpl.csv", rows)
        self.assert_rejected()

    def test_unquoted_grouped_designator_fails(self) -> None:
        path = self.board / "jlc_bom.csv"
        path.write_text(path.read_text(encoding="utf-8").replace(
            '100k 1%,"R4,R5",R_0603,C14675',
            '100k 1%,R4,R5,R_0603,C14675'), encoding="utf-8")
        self.assert_rejected()

    def test_value_footprint_lcsc_side_coordinate_and_rotation_drift_fail(self) -> None:
        cases = (
            ("bom", "Comment", "wrong value"),
            ("bom", "Footprint", "wrong_footprint"),
            ("bom", "LCSC Part #", "C00000"),
            ("cpl", "Layer", "Bottom"),
            ("cpl", "Mid X", "18.100000"),
            ("cpl", "Rotation", "91.000000"),
        )
        for kind, column, value in cases:
            with self.subTest(kind=kind, column=column):
                path = self.board / ("jlc_bom.csv" if kind == "bom" else "jlc_cpl.csv")
                original = path.read_text(encoding="utf-8")
                rows = read_rows(path)
                header = rows[0]
                index = header.index(column)
                target = next(row for row in rows[1:]
                              if (row[-1] == "C14675" if kind == "bom" else row[0] == "R4"))
                target[index] = value
                write_rows(path, rows)
                self.assert_rejected()
                path.write_text(original, encoding="utf-8")

    def test_u1_native_rotation_and_substitution_fail(self) -> None:
        rows = read_rows(self.board / "jlc_cpl.csv")
        next(row for row in rows if row and row[0] == "U1")[-1] = "0.000000"
        write_rows(self.board / "jlc_cpl.csv", rows)
        self.assert_rejected()

        shutil.copy2(ROOT / "hardware" / "rp2350_zero_carrier" / "jlc_cpl.csv",
                     self.board / "jlc_cpl.csv")
        manifest = self.board / "jlc_assembly.json"
        manifest.write_text(manifest.read_text(encoding="utf-8").replace(
            '"mpn": "TPS2553DBVR"', '"mpn": "TPS2553DBVR-1"', 1),
            encoding="utf-8")
        self.assert_rejected()

        shutil.copy2(ROOT / "hardware" / "rp2350_zero_carrier" / "jlc_assembly.json",
                     manifest)
        manifest.write_text(manifest.read_text(encoding="utf-8").replace(
            '"lcsc": "C55266"', '"lcsc": "C00000"', 1), encoding="utf-8")
        self.assert_rejected()


if __name__ == "__main__":
    unittest.main(verbosity=2)
