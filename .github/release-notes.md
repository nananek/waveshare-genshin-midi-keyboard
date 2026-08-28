## Hardware and firmware

- `genshin_midi_kbd.uf2` is for board 1: USB MIDI to HID keyboard, with a raw
  UART1 MIDI mirror.
- `serial_midi_device.uf2` is for board 2: UART1 to USB-MIDI bridge.  Connect
  board 1 GP4 to board 2 GP5 and connect their grounds.
- The RP2350-Zero carrier manufacturing archive, JLCPCB BOM/CPL assembly CSVs,
  DRC report, schematic PDF, board-layout PDF, and checksums are included with
  this release.  The Gerbers and drawings were generated from this tag with
  KiCad 10.0.5.

## Assembly note

When using board 1 as a USB host, remove the RP2350-Zero's D+ pull-up resistor
R13.  Leaving R13 fitted prevents correct MIDI-keyboard enumeration and hot-plug
detection.  See the README for wiring and switch details.
