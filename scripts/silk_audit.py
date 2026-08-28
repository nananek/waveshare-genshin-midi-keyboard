#!/usr/bin/env python3
"""Fail when front/back silkscreen items overlap or leave the board.

Run with the same Python environment that provides KiCad's ``pcbnew`` module:
    python3 scripts/silk_audit.py hardware/rp2350_zero_carrier/rp2350_zero_carrier.kicad_pcb

This deliberately checks the bounding boxes of text and graphics.  It is a
conservative companion to KiCad DRC: it catches footprint-to-footprint silk
collisions, which DRC may not report consistently across KiCad versions.
"""

from __future__ import annotations

import sys

import pcbnew


SILK_LAYERS = {
    pcbnew.F_SilkS: "F.SilkS",
    pcbnew.B_SilkS: "B.SilkS",
}


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
    # KiCad 10's SWIG iterator is not compatible with Python 3.14 because it
    # still exposes ``next`` instead of ``__next__``. Index SWIG containers so
    # the audit also runs in the pinned KiCad 10.0 release environment.
    drawings = board.Drawings()
    edge_boxes = [bounds(drawings[index]) for index in range(len(drawings))
                  if drawings[index].GetLayer() == pcbnew.Edge_Cuts]
    if not edge_boxes:
        print("FAIL: board has no Edge.Cuts", file=sys.stderr)
        return 1

    # This board has a rectangular outline.  Its enclosing bounds are the
    # applicable keep-in region for every silkscreen item.
    left = min(box[0] for box in edge_boxes)
    top = min(box[1] for box in edge_boxes)
    right = max(box[2] for box in edge_boxes)
    bottom = max(box[3] for box in edge_boxes)

    silk = {layer: [] for layer in SILK_LAYERS}
    failures = []
    for footprint in board.GetFootprints():
        reference = footprint.GetReference()
        graphics = footprint.GraphicalItems()
        items = [(f"graphic[{index}]", graphics[index])
                 for index in range(len(graphics))]
        items.extend((name, field) for name, field in (
            ("Reference", footprint.Reference()),
            ("Value", footprint.Value()),
        ) if field.IsVisible())
        for item_name, item in items:
            layer = item.GetLayer()
            if layer not in SILK_LAYERS:
                continue
            layer_name = SILK_LAYERS[layer]
            item_bounds = bounds(item)
            label = item.GetText() if hasattr(item, "GetText") else item.GetClass()
            if (item_bounds[0] < left or item_bounds[1] < top or
                    item_bounds[2] > right or item_bounds[3] > bottom):
                failures.append(
                    f"{reference}: {layer_name} {item_name} {label!r} leaves Edge.Cuts"
                )
            silk[layer].append((reference, item_name, label, item_bounds))

    for layer, layer_items in silk.items():
        layer_name = SILK_LAYERS[layer]
        for index, (reference, item_name, label, item_bounds) in enumerate(layer_items):
            for (other_reference, other_item_name, other_label,
                 other_bounds) in layer_items[index + 1:]:
                if reference != other_reference and overlaps(item_bounds, other_bounds):
                    failures.append(
                        f"{reference}: {layer_name} {item_name} {label!r} overlaps "
                        f"{other_reference}: {other_item_name} {other_label!r}"
                    )

    if failures:
        print("FAIL: silkscreen audit found:", file=sys.stderr)
        print("\n".join(f"  - {failure}" for failure in failures), file=sys.stderr)
        return 1
    print("PASS: F.SilkS and B.SilkS stay inside Edge.Cuts and have no "
          "same-layer cross-footprint overlaps")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
