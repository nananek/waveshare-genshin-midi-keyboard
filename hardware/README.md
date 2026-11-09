# Hardware

| Directory | Status | Summary |
|-----------|--------|---------|
| `rp2350_frisk/` | **Draft (routing WIP)** | Current direction. 57.6 × 31.4 mm Frisk-size board, bare RP2350A QFN-60, CH9329, USB-C ×2 + USB-A ×1. |
| `rp2350_zero_carrier/` | **Legacy** | Old 2-board design with two socketed RP2350-Zero modules. Kept for history. Not built by the current firmware or CI release target. |

The current firmware (`midi_ch9329_gadget.uf2`) targets the new `rp2350_frisk`
pin assignment. The old carrier's manufacturing data and contract tests remain
in the repository as legacy artifacts; do not mix them with the new board.

## New board TODO

`hardware/rp2350_frisk/` has a complete outline, component placement, and net
assignment, but no routed copper or schematic yet. See its `README.md` for the
pin map, power tree, and the list of work required before fabrication.
