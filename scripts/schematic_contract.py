#!/usr/bin/env python3
"""Validate native annotation exports and the project-local module symbol."""

from __future__ import annotations

import csv
from pathlib import Path
import re
import sys


EXPECTED_REFS = {
    "C1", "C2", "C3", "F1", "J1", "J2", "J3", "R1", "R2", "R3",
    "R4", "R5", "SW1", "SW2", "SW3", "SW4", "SW5", "SW6", "U1",
}
EXPECTED_MODULE_PINS = {
    "1": ("5V", "power_out"),
    "2": ("GND", "passive"),
    "3": ("3V3", "power_out"),
    "4": ("GP29", "bidirectional"),
    "5": ("GP28", "bidirectional"),
    "6": ("GP27", "bidirectional"),
    "7": ("GP26", "bidirectional"),
    "8": ("GP15", "bidirectional"),
    "9": ("GP14", "bidirectional"),
    "10": ("GP0", "bidirectional"),
    "11": ("GP1", "bidirectional"),
    "12": ("GP2", "bidirectional"),
    "13": ("GP3", "bidirectional"),
    "14": ("GP4", "bidirectional"),
    "15": ("GP5", "bidirectional"),
    "16": ("GP6", "bidirectional"),
    "17": ("GP7", "bidirectional"),
    "18": ("GP8", "bidirectional"),
    "19": ("GP13", "bidirectional"),
    "20": ("GP12", "bidirectional"),
    "21": ("GP11", "bidirectional"),
    "22": ("GP10", "bidirectional"),
    "23": ("GP9", "bidirectional"),
}


def fail(message: str) -> None:
    raise SystemExit(f"SCHEMATIC CONTRACT FAIL: {message}")


def form_at(source: str, start: int) -> str:
    """Return one balanced s-expression, respecting quoted strings."""
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
    fail("unbalanced s-expression")


def symbol_pins(source: str) -> dict[str, tuple[str, str]]:
    marker = '(symbol "RP2350_Zero_Header_23p_1_1"'
    start = source.find(marker)
    if start < 0:
        fail("RP2350-Zero 23-pin symbol body is missing")
    symbol = form_at(source, start)
    pins: dict[str, tuple[str, str]] = {}
    position = 0
    while True:
        start = symbol.find("(pin ", position)
        if start < 0:
            break
        pin = form_at(symbol, start)
        type_match = re.match(r"\(pin\s+(\S+)\s+", pin)
        name_match = re.search(r'\(name\s+"([^"]+)"', pin)
        number_match = re.search(r'\(number\s+"([^"]+)"', pin)
        if not (type_match and name_match and number_match):
            fail(f"cannot parse module pin: {pin[:80]}")
        number = number_match.group(1)
        if number in pins:
            fail(f"duplicate module pin {number}")
        pins[number] = (name_match.group(1), type_match.group(1))
        position = start + len(pin)
    return pins


def main() -> None:
    if len(sys.argv) != 5:
        fail("usage: schematic_contract.py SCHEMATIC SYMBOL_LIB NETLIST BOM")
    schematic_path, symbol_path, netlist_path, bom_path = map(Path, sys.argv[1:])
    schematic = schematic_path.read_text(encoding="utf-8")
    symbol_library = symbol_path.read_text(encoding="utf-8")
    netlist = netlist_path.read_text(encoding="utf-8")

    if '(paper "A4")' not in schematic:
        fail("schematic source is not native A4")

    with bom_path.open(newline="", encoding="utf-8-sig") as source:
        bom_refs = {row["Refs"] for row in csv.DictReader(source)}
    netlist_refs = set(re.findall(r'\(ref\s+"([^"]+)"\)', netlist))
    instance_refs = set(re.findall(r'\(reference\s+"([^"]+)"\)', schematic))
    for origin, references in (
            ("native BOM", bom_refs), ("native netlist", netlist_refs),
            ("schematic instances", instance_refs)):
        if references != EXPECTED_REFS:
            fail(f"{origin} references: expected {sorted(EXPECTED_REFS)}, "
                 f"got {sorted(references)}")
        invalid = sorted(ref for ref in references
                         if not re.fullmatch(r"[A-Z]+[0-9]+", ref))
        if invalid:
            fail(f"{origin} contains unannotated/nonstandard refs: {invalid}")
    if "?" in "".join(sorted(bom_refs)) or "?" in "".join(sorted(netlist_refs)):
        fail("native export contains '?' reference")

    library_pins = symbol_pins(symbol_library)
    embedded_pins = symbol_pins(schematic)
    if library_pins != EXPECTED_MODULE_PINS:
        fail(f"project-local module pins: {library_pins}")
    if embedded_pins != EXPECTED_MODULE_PINS:
        fail(f"embedded module pins: {embedded_pins}")
    if library_pins != embedded_pins:
        fail("project-local and embedded module pin definitions differ")

    print("SCHEMATIC CONTRACT PASS")
    print("  native BOM/netlist annotation: 19 unique standard references, no '?'")
    print("  RP2350-Zero pins 1..23: 5V,GND,3V3,GP29..GP9")
    print("  power pins typed; GPIO pins bidirectional; embedded copy synchronized")


if __name__ == "__main__":
    main()
