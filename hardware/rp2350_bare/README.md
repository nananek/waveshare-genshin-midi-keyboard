# RP2350A Bare Minimal Baseline

This directory is based on Raspberry Pi's official `RP2350A QFN-60 Minimal
Design Example R4` (KiCad project), imported from `Minimal-KiCAD.zip`.
The project-local `fp-lib-table` and `RPI_COMPILED.pretty` preserve the
official PCB footprint names while making the imported board resolvable
without the original Raspberry Pi database installation.

It is the electrical baseline for the 60 x 60 mm board. The source
contains the bare RP2350A, external `W25Q128JVSIQ` QSPI flash, 12 MHz crystal,
3.3 V supply, internal core-regulator network, RUN/BOOTSEL, SWD, USB device,
and the official decoupling/layout patterns. `official-minimal-LICENSE.txt`
is retained with the source.

The imported layout remains 2-layer (F.Cu/B.Cu), but the Edge.Cuts were
expanded to 60 x 60 mm (70..130 x 66..126, 3 mm corner radius) while retaining
the official placement and routing. A 4-layer conversion is deferred until a
fabricator stackup is selected and the regulator, crystal, QSPI, and USB
return paths are reviewed. The USB-A PIO host/current-limit/ESD and
application controls are not yet part of this baseline; they must be added
from a finalized schematic/netlist rather than guessed into the PCB. The
official baseline already contains native USB, RUN/BOOTSEL and SWD; UART and
an LED are not present in the official Minimal and remain intentionally
unadded.

The cleanup verification on 2026-08-27 with KiCad 10.0.5 reports 163 DRC
violations and 0 unconnected pads, improved from the prior 208/24 result.
All PCB references are unique and host routing is on B.Cu with GND stitching
vias. The supplied schematic netlist exports with its pre-existing annotation
warning; the added R11/R12/F1 implementation footprints are documented in the
verification record for subsequent full GUI parity review. Gerbers, Excellon
drill, schematic PDF, and reports are under `release_artifacts/`.

Do not use `hardware/rp2350_smt_compact` as a source: that design is the
discarded RP2350-Zero-module prototype.
