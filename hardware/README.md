# Hardware

| Directory | Status | Summary |
|-----------|--------|---------|
| `rp2350_frisk/` | **Fabrication data ready (6-layer)** | Current board. 57.6 × 31.4 mm Frisk size, bare RP2350A QFN-60, CH345T, USB-C ×2 + USB-A ×1. DRC 0 / 0. |
| `rp2350_zero_carrier/` | **Legacy** | Old 2-board design with two socketed RP2350-Zero modules. Kept for history. Not built by the current firmware or CI release target. |

The current firmware (`midi_nkro_ch345.uf2`) targets `rp2350_frisk`:

- native USB = NKRO HID keyboard
- PIO-USB host = MIDI keyboard
- UART1 31250 = CH345T USB-MIDI

The old carrier's manufacturing data and contract tests remain in the repository
as legacy artifacts; do not mix them with the new board.

## Ordering

`hardware/rp2350_frisk/fab/` contains the Gerber ZIP, JLCPCB BOM/CPL, DRC report,
and `ORDER_NOTES_JA.md`. Note that U1 pin22 and J3 A7/B7 use via-in-pad; confirm
resin fill / via-in-pad requirements in JLCPCB's DFM preview. BOM rows marked
`TBD` must be filled before PCBA ordering.
