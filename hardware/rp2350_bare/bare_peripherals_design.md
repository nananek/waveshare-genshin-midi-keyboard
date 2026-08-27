# Bare RP2350 peripheral implementation record

This is the approved electrical definition for the next schematic/PCB edit.
It deliberately names the parts and nets before placement so the USB host is
not added by guessing.

## Parts

- J2: Neltron 5075AR-04-WH USB-A receptacle
- U3: TI TPS2051BDBVR, 5-pin high-side USB power switch, 500 mA limit
- U4: ST USBLC6-2SC6, USB D+/D- ESD protector
- F1: resettable 500 mA PTC on incoming VBUS, selected to match the source
- R3/R4: 22 ohm 0603 series resistors on D+/D-
- J4: 1x8 2.54 mm UART/control header

The corresponding USB-A, TPS2051B, USBLC6, 0603 resistor/capacitor, fuse,
and UART test/header footprints are present in `RPI_COMPILED.pretty`.

## Required nets

```
RP2350 /GPIO12 -- R3 22R -- USB_A_DP -- U4 IO1 -- J2 pin 3 (D+)
RP2350 /GPIO13 -- R4 22R -- USB_A_DM -- U4 IO2 -- J2 pin 2 (D-)
VBUS -- F1 -- TPS2051B IN; TPS2051B OUT -- USB_A_VBUS -- J2 pin 1
U3 EN/ILIM -- GND (always-on, per selected TPS2051B variant)
U4 VCC/VBUS -- USB_A_VBUS; U4 GND -- GND; J2 pin 4 and shell -- GND
```

J4 exposes GP0/UART0_TX, GP1/UART0_RX, GP4, GP5, 3V3, GND, RUN and SWDIO;
the official BOOTSEL/RUN/SWD circuitry remains unchanged. Native USB-C keeps
the official Minimal USB_DP/USB_DM/VBUS/GND wiring.

## Current status

The exact part/connection definition and local footprints are staged, but the
new symbols, PCB instances, and routed copper are not yet merged into the
official Minimal schematic/PCB. This record must not be treated as a
manufacturable host design until schematic-PCB parity and ERC/DRC both pass.
