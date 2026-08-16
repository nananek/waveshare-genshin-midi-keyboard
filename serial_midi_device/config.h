#ifndef CONFIG_H
#define CONFIG_H

// ===========================================================================
//  シリアル→USB-MIDI ブリッジ設定 (ボード2 / Waveshare RP2350-USB-A)
//  A 側のミラー (UART1 TX=GP4 @31250) を UART1 RX=GP5 で受けて、
//  native USB (USB-C) を USB-MIDI デバイスとして PC/DAW に見せる。
//  すべて #ifndef ガード付き。デバッグログは UART0 (GP0/GP1) なので衝突しない。
// ===========================================================================

#ifndef MIDI_UART_RX_PIN
#define MIDI_UART_RX_PIN 5        // UART1 RX (ミラー側 TX=GP4 と直結)
#endif
#ifndef MIDI_UART_BAUD
#define MIDI_UART_BAUD 31250      // 標準 MIDI over UART (8N1)
#endif
// 0 = uart0 / 1 = uart1 (デバッグ uart0 と分離するため 1 が既定)
#ifndef MIDI_UART_INDEX
#define MIDI_UART_INDEX 1
#endif

#endif // CONFIG_H
