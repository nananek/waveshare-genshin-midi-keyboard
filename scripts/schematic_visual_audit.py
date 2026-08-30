#!/usr/bin/env python3
"""Gate the reviewed A4 schematic placement and rendered golden image."""

from __future__ import annotations

import hashlib
import math
from pathlib import Path
import re
import subprocess
import sys
import tempfile


FIELD_DISTANCE_LIMITS_MM = {
    "J1": 40.0, "J2": 35.0, "J3": 40.0,
    "SW1": 35.0, "SW2": 35.0, "SW3": 35.0,
    "SW4": 35.0, "SW5": 35.0, "SW6": 30.0,
    "U1": 15.0, "C1": 15.0, "C2": 15.0, "C3": 15.0,
    "R3": 15.0, "R4": 15.0, "R5": 15.0,
}


def fail(message: str) -> None:
    raise SystemExit(f"SCHEMATIC VISUAL AUDIT FAIL: {message}")


def balanced_form(source: str, start: int) -> str:
    depth = 0
    quoted = False
    escaped = False
    for index in range(start, len(source)):
        character = source[index]
        if quoted:
            if escaped:
                escaped = False
            elif character == "\\":
                escaped = True
            elif character == '"':
                quoted = False
            continue
        if character == '"':
            quoted = True
        elif character == "(":
            depth += 1
        elif character == ")":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    fail("unbalanced schematic s-expression")


def field_positions(schematic: str) -> dict[str, tuple[tuple[float, float],
                                                        tuple[float, float],
                                                        tuple[float, float]]]:
    positions = {}
    offset = 0
    while True:
        start = schematic.find("(symbol (lib_id ", offset)
        if start < 0:
            break
        form = balanced_form(schematic, start)
        header = re.match(
            r'\(symbol\s+\(lib_id\s+"[^"]+"\)\s+'
            r'\(at\s+([-0-9.]+)\s+([-0-9.]+)', form)
        reference = re.search(
            r'\(property\s+"Reference"\s+"([^"]+)"\s+'
            r'\(at\s+([-0-9.]+)\s+([-0-9.]+)', form)
        value = re.search(
            r'\(property\s+"Value"\s+"[^"]*"\s+'
            r'\(at\s+([-0-9.]+)\s+([-0-9.]+)', form)
        if header and reference and value:
            positions[reference.group(1)] = (
                (float(header.group(1)), float(header.group(2))),
                (float(reference.group(2)), float(reference.group(3))),
                (float(value.group(1)), float(value.group(2))),
            )
        offset = start + len(form)
    return positions


def main() -> None:
    if len(sys.argv) != 4:
        fail("usage: schematic_visual_audit.py SCHEMATIC PDF GOLDEN_SHA256")
    schematic_path, pdf_path, golden_path = map(Path, sys.argv[1:])
    schematic = schematic_path.read_text(encoding="utf-8")
    positions = field_positions(schematic)

    for reference, limit in FIELD_DISTANCE_LIMITS_MM.items():
        if reference not in positions:
            fail(f"missing placement for {reference}")
        symbol, reference_field, value_field = positions[reference]
        for field_name, field in (("Reference", reference_field),
                                  ("Value", value_field)):
            distance = math.dist(symbol, field)
            if distance > limit:
                fail(f"{reference} {field_name} is {distance:.1f}mm from symbol "
                     f"(limit {limit:.1f}mm)")
        if math.dist(reference_field, value_field) < 2.0:
            fail(f"{reference} Reference/Value anchors overlap")

    with tempfile.TemporaryDirectory() as directory:
        rendered = Path(directory) / "schematic-a4-180dpi.pgm"
        subprocess.run([
            "gs", "-q", "-dSAFER", "-dBATCH", "-dNOPAUSE",
            "-sDEVICE=pgmraw", "-r180", "-dFirstPage=1", "-dLastPage=1",
            f"-sOutputFile={rendered}", str(pdf_path),
        ], check=True)
        digest = hashlib.sha256(rendered.read_bytes()).hexdigest()

        text = subprocess.run([
            "gs", "-q", "-dSAFER", "-dBATCH", "-dNOPAUSE",
            "-sDEVICE=txtwrite", "-sOutputFile=-", str(pdf_path),
        ], check=True, capture_output=True, text=True).stdout
        merged_pin_text = re.findall(
            r"\b(?:1IN|2GND|3EN|4FAULT|5ILIM|6OUT)\b", text)
        if merged_pin_text:
            fail(f"merged U1 pin-number/name text: {merged_pin_text}")
        required_text = (
            "RP2350_Zero_Header_23p", "SW_SPDT_NKK_115643_Slide",
            "SW_TACT_Akizuki_DTS63", "TPS2553DBVR", "100nF X7R 50V",
            "100k EN pulldown",
        )
        missing = [token for token in required_text if token not in text]
        if missing:
            fail(f"rendered PDF is missing visible fields: {missing}")

    golden_line = golden_path.read_text(encoding="ascii").strip()
    expected = golden_line.split()[0] if golden_line else ""
    if not re.fullmatch(r"[0-9a-f]{64}", expected):
        fail(f"invalid golden digest file: {golden_path}")
    if digest != expected:
        fail(f"rendered A4 golden changed: expected {expected}, got {digest}")

    print("SCHEMATIC VISUAL AUDIT PASS")
    print("  A4 field proximity: J1/J3/SW1-SW6/U1/R/C")
    print("  rendered required text present; no merged U1 pin text")
    print(f"  reviewed 180dpi grayscale golden: {digest}")


if __name__ == "__main__":
    main()
