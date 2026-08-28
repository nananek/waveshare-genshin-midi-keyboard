#!/usr/bin/env python3
"""Fail when F.SilkS graphics overlap another footprint or leave the board.

Run with the same Python environment that provides KiCad's ``pcbnew`` module:
    python3 scripts/silk_audit.py hardware/rp2350_zero_carrier/rp2350_zero_carrier.kicad_pcb

This deliberately checks the bounding boxes of text and graphics.  It is a
conservative companion to KiCad DRC: it catches footprint-to-footprint silk
collisions, which DRC may not report consistently across KiCad versions.
"""

from __future__ import annotations

import sys

import pcbnew

# KiCad 9's generated Python bindings still call the Python 2-style ``next``
# method internally.  Python 3.14 exposes only ``__next__`` on this SWIG
# iterator, so restore the alias before using BOARD collection helpers.
if not hasattr(pcbnew.SwigPyIterator, "next"):
    pcbnew.SwigPyIterator.next = pcbnew.SwigPyIterator.__next__


def bounds(item):
    box = item.GetBoundingBox()
    return box.GetX(), box.GetY(), box.GetRight(), box.GetBottom()


def overlaps(first, second):
    ax1, ay1, ax2, ay2 = first
    bx1, by1, bx2, by2 = second
    return ax1 < bx2 and bx1 < ax2 and ay1 < by2 and by1 < ay2


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} BOARD.kicad_pcb", file=sys.stderr)
        return 2

    board = pcbnew.LoadBoard(sys.argv[1])
    edge_boxes = [bounds(item) for item in board.GetDrawings()
                  if item.GetLayer() == pcbnew.Edge_Cuts]
    if not edge_boxes:
        print("FAIL: board has no Edge.Cuts", file=sys.stderr)
        return 1

    # This board has a rectangular outline.  Its enclosing bounds are the
    # applicable keep-in region for every F.SilkS item.
    left = min(box[0] for box in edge_boxes)
    top = min(box[1] for box in edge_boxes)
    right = max(box[2] for box in edge_boxes)
    bottom = max(box[3] for box in edge_boxes)

    silk = []
    failures = []
    for footprint in board.GetFootprints():
        reference = footprint.GetReference()
        for index, item in enumerate(footprint.GraphicalItems()):
            if item.GetLayer() != pcbnew.F_SilkS:
                continue
            item_bounds = bounds(item)
            label = item.GetText() if hasattr(item, "GetText") else item.GetClass()
            if (item_bounds[0] < left or item_bounds[1] < top or
                    item_bounds[2] > right or item_bounds[3] > bottom):
                failures.append(f"{reference}: F.SilkS {label!r} leaves Edge.Cuts")
            silk.append((reference, index, label, item_bounds))

    for index, (reference, _, label, item_bounds) in enumerate(silk):
        for other_reference, _, other_label, other_bounds in silk[index + 1:]:
            if reference != other_reference and overlaps(item_bounds, other_bounds):
                failures.append(
                    f"{reference}: F.SilkS {label!r} overlaps "
                    f"{other_reference}: {other_label!r}"
                )

    if failures:
        print("FAIL: silkscreen audit found:", file=sys.stderr)
        print("\n".join(f"  - {failure}" for failure in failures), file=sys.stderr)
        return 1
    print("PASS: F.SilkS stays inside Edge.Cuts and has no cross-footprint overlaps")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
