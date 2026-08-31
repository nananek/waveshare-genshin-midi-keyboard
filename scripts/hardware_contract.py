#!/usr/bin/env python3
"""Fail closed when the reviewed power/switch/JLC hardware contract drifts."""

from __future__ import annotations

import heapq
import math
from pathlib import Path
import sys

import pcbnew

from generate_jlc_assembly import load_manifest
from jlc_assembly_contract import validate_jlc_assembly


EXPECTED_PADS = {
    "U1": {"1": "VBUS_TPS_IN", "2": "GND", "3": "USB_PWR_EN",
           "4": "USB_PWR_FAULT_N", "5": "TPS_ILIM", "6": "VBUS_USB_A"},
    "J1": {"1": "VBUS_5V", "2": "GND", "3": "3V3",
           "4": "GP29_SW2_LYRE_MODE", "5": "GP28_SW1_GAME_MODE",
           "6": "GP27_SW3_SPARE", "7": "", "8": "", "9": "",
           "10": "GP0_UART0_TX", "11": "GP1_UART0_RX", "12": "",
           "13": "", "14": "UART_MIRROR", "15": "UART_MIRROR_RETURN",
           "16": "", "17": "", "18": "", "19": "GP13_USB_DM",
           "20": "GP12_USB_DP", "21": "", "22": "USB_PWR_FAULT_N",
           "23": "USB_PWR_EN"},
    "J3": {"1": "", "2": "GND", "3": "", "4": "GP29_SW5_SUSTAIN",
           "5": "GP28_SW6_RESET", "6": "", "7": "", "8": "", "9": "",
           "10": "J3_GP0_UART0_TX", "11": "J3_GP1_UART0_RX", "12": "",
           "13": "", "14": "UART_MIRROR_RETURN", "15": "UART_MIRROR",
           "16": "", "17": "", "18": "", "19": "", "20": "",
           "21": "", "22": "", "23": ""},
    "J2": {"1": "VBUS_USB_A_SW", "2": "USB_A_DM", "3": "USB_A_DP",
           "4": "GND", "5": "GND", "6": "GND"},
    "F1": {"1": "VBUS_5V", "2": "VBUS_TPS_IN"},
    "R1": {"1": "GP12_USB_DP", "2": "USB_A_DP"},
    "R2": {"1": "GP13_USB_DM", "2": "USB_A_DM"},
    "R5": {"1": "USB_PWR_EN", "2": "GND"},
    "R4": {"1": "3V3", "2": "USB_PWR_FAULT_N"},
    "R3": {"1": "GND", "2": "TPS_ILIM"},
    "C1": {"1": "GND", "2": "VBUS_TPS_IN"},
    "C3": {"1": "VBUS_USB_A", "2": "GND"},
    "C2": {"1": "VBUS_USB_A_SW", "2": "GND"},
    "SW1": {"1": "", "2": "GP28_SW1_GAME_MODE", "3": "GND"},
    "SW2": {"1": "", "2": "GP29_SW2_LYRE_MODE", "3": "GND"},
    "SW3": {"1": "", "2": "GP27_SW3_SPARE", "3": "GND"},
    "SW4": {"1": "", "2": "VBUS_USB_A", "3": "VBUS_USB_A_SW"},
    "SW5": {"1": "", "2": "GP29_SW5_SUSTAIN", "3": "GND"},
    "SW6": {"1": "GND", "2": "GND", "3": "GP28_SW6_RESET",
            "4": "GP28_SW6_RESET"},
    "TP_EN": {"1": "USB_PWR_EN"},
    "TP_FAULT": {"1": "USB_PWR_FAULT_N"},
    "TP_IN": {"1": "VBUS_TPS_IN"},
    "TP_OUT": {"1": "VBUS_USB_A"},
    "TP_GND": {"1": "GND"},
}

# x/y use JLC's Cartesian convention: PCB editor X, inverted editor Y. These
# reviewed physical placements remain independent of the native-derived CPL.
EXPECTED_ASSEMBLY_PLACEMENTS = {
    "U1": (11.0, 17.3, 0.0),
    "C1": (8.5, 21.0, 180.0),
    "R3": (14.5, 17.8, 180.0),
    "R4": (18.0, 12.0, 90.0),
    "R5": (5.5, 13.5, 0.0),
    "C3": (15.25, 20.8, 90.0),
}

TEST_PAD_POSITIONS = {
    "TP_EN": (8.0, -12.5),
    "TP_FAULT": (14.5, -11.5),
    "TP_IN": (6.0, -21.5),
    "TP_OUT": (18.0, -20.05),
    "TP_GND": (12.0, -21.5),
}


def fail(message: str) -> None:
    raise SystemExit(f"HARDWARE CONTRACT FAIL: {message}")


def pad_map(board: pcbnew.BOARD, reference: str) -> dict[str, str]:
    footprint = board.FindFootprintByReference(reference)
    if footprint is None:
        fail(f"missing footprint {reference}")
    pads = footprint.Pads()
    return {str(pads[index].GetNumber()): pads[index].GetNetname()
            for index in range(pads.size())}


def shortest_track_path_mm(board: pcbnew.BOARD, net_name: str,
                           start: tuple[float, float],
                           end: tuple[float, float],
                           layer_name: str) -> float:
    """Return the routed centerline length on one copper layer, excluding vias."""
    graph: dict[tuple[float, float], list[tuple[tuple[float, float], float]]] = {}
    tracks = board.Tracks()
    for index in range(tracks.size()):
        track = tracks[index]
        if (track.GetNetname() != net_name or
                isinstance(track, pcbnew.PCB_VIA) or
                track.GetLayerName() != layer_name):
            continue
        track_start = track.GetStart()
        track_end = track.GetEnd()
        first = (round(pcbnew.ToMM(track_start.x), 6),
                 round(pcbnew.ToMM(track_start.y), 6))
        second = (round(pcbnew.ToMM(track_end.x), 6),
                  round(pcbnew.ToMM(track_end.y), 6))
        length = math.dist(first, second)
        graph.setdefault(first, []).append((second, length))
        graph.setdefault(second, []).append((first, length))

    queue = [(0.0, start)]
    visited: set[tuple[float, float]] = set()
    while queue:
        distance, point = heapq.heappop(queue)
        if point in visited:
            continue
        if point == end:
            return distance
        visited.add(point)
        for neighbour, length in graph.get(point, []):
            heapq.heappush(queue, (distance + length, neighbour))
    fail(f"no routed {net_name} path between {start} and {end}")


def main() -> None:
    board_dir = Path(sys.argv[1] if len(sys.argv) > 1
                     else "hardware/rp2350_zero_carrier")
    board_path = board_dir / f"{board_dir.name}.kicad_pcb"
    board = pcbnew.LoadBoard(str(board_path))

    for reference, expected in EXPECTED_PADS.items():
        actual = pad_map(board, reference)
        if actual != expected:
            fail(f"{reference} pads: expected {expected}, got {actual}")

    # The two upstream-device headers must never receive downstream USB VBUS.
    for reference in ("J1", "J3"):
        actual = set(pad_map(board, reference).values())
        if "VBUS_USB_A" in actual or "VBUS_USB_A_SW" in actual:
            fail(f"{reference} is tied to downstream USB-A VBUS")
    if pad_map(board, "J3")["1"] or pad_map(board, "J3")["3"]:
        fail("J3 VBUS/3V3 isolation changed")

    for reference, expected_position in TEST_PAD_POSITIONS.items():
        footprint = board.FindFootprintByReference(reference)
        if not (footprint.IsBoardOnly() and footprint.IsExcludedFromBOM()
                and footprint.IsExcludedFromPosFiles()):
            fail(f"{reference} must remain board-only and BOM/CPL-excluded")
        position = footprint.GetPosition()
        actual_position = (round(pcbnew.ToMM(position.x), 6),
                           round(pcbnew.ToMM(position.y), 6))
        if actual_position != expected_position:
            fail(f"{reference} position: expected {expected_position}, "
                 f"got {actual_position}")

    components = load_manifest(board_dir)
    assembly = {str(component["reference"]): component
                for component in components}
    if set(assembly) != set(EXPECTED_ASSEMBLY_PLACEMENTS):
        fail(f"JLC assembly manifest references: {sorted(assembly)}")
    # This expands the grouped BOM designators before comparing them to the
    # six distinct CPL placements and the KiCad-native position export.
    validate_jlc_assembly(board_dir)

    decision = (board_dir / "ORDER_DECISION_JA.md").read_text(encoding="utf-8")
    exact_rows = 0
    for reference, expected in assembly.items():
        row_prefix = (f"| {reference} | {expected['manufacturer']} | "
                      f"{expected['mpn']} | {expected['lcsc']} |")
        if decision.count(row_prefix) != 1:
            fail(f"missing or duplicate exact order-table row: {reference}")
        exact_rows += 1
    if exact_rows != 6:
        fail(f"exact Manufacturer/MPN/LCSC table has {exact_rows} rows")
    if "TPS2553DBVR-1" not in decision or "一切の代替を禁止" not in decision:
        fail("TPS2553DBVR-1/substitution ban is missing from order decision")

    for reference, expected in assembly.items():
        board_footprint = board.FindFootprintByReference(reference)
        if board_footprint.GetValue() != expected["pcb_value"]:
            fail(f"{reference} PCB value: expected {expected['pcb_value']!r}, "
                 f"got {board_footprint.GetValue()!r}")
        if board_footprint.GetFPIDAsString().split(":")[-1] != expected["footprint"]:
            fail(f"{reference} PCB footprint: expected {expected['footprint']!r}, "
                 f"got {board_footprint.GetFPIDAsString()!r}")
        position = board_footprint.GetPosition()
        board_placement = (
            round(pcbnew.ToMM(position.x), 6),
            round(-pcbnew.ToMM(position.y), 6),
            round(board_footprint.GetOrientationDegrees() % 360.0, 6),
        )
        expected_board_placement = EXPECTED_ASSEMBLY_PLACEMENTS[reference]
        if board_placement != expected_board_placement:
            fail(f"{reference} PCB placement: expected "
                 f"{expected_board_placement}, got {board_placement}")

    # TI calls for short IN-GND and OUT-GND bypass loops. These bounds lock the
    # directly routed F.Cu branches; DRC and zone refill independently cover
    # copper clearance and the wider ground-plane connectivity.
    input_path = shortest_track_path_mm(
        board, "VBUS_TPS_IN", (7.75, -21.0), (9.7, -18.25), "F.Cu")
    input_ground_path = shortest_track_path_mm(
        board, "GND", (9.25, -21.0), (9.7, -17.3), "F.Cu")
    output_path = shortest_track_path_mm(
        board, "VBUS_USB_A", (12.3, -18.5), (15.25, -20.05), "F.Cu")
    if input_path > 4.0:
        fail(f"C1-to-IN routed path too long: {input_path:.3f} mm")
    if input_ground_path > 6.0:
        fail(f"C1-to-GND routed path too long: {input_ground_path:.3f} mm")
    if output_path > 5.0:
        fail(f"OUT-to-C3 routed path too long: {output_path:.3f} mm")

    print("HARDWARE CONTRACT PASS")
    print("  EN=J1.23/GP9, FAULT=J1.22/GP10, FAULT pull-up=3V3")
    print("  boot default off=R5 100k; OUT HF bypass=C3 100nF")
    print(f"  F.Cu bypass paths: C1-IN={input_path:.3f} mm, "
          f"C1-GND={input_ground_path:.3f} mm, "
          f"OUT-C3={output_path:.3f} mm")
    print("  J3 pad1/pad3 isolated; SW3 spare; SW4 manual VBUS switch")
    print(f"  JLC BOM/CPL exact assembly set: {', '.join(sorted(assembly))}")
    print("  JLC BOM materials=5; C14675 is one R4,R5 row (quantity 2)")
    print("  exact Manufacturer/MPN/LCSC order table: 6 rows; substitution ban present")


if __name__ == "__main__":
    main()
