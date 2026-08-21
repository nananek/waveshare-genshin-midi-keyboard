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

// ---------------------------------------------------------------------------
//  MIDI RESETタクトスイッチ (issue #6)
//  押下(デバウンス確定)の立ち上がりエッジごとに MIDI System Reset (0xFF、
//  1バイトSystem Realtimeメッセージ) を1回送信する。ボード1のMUTE_SWITCH_PIN
//  (GP28、src/config.h)と同一物理ピン番号に揃えている(部品・配線の使い回し用)。
// ---------------------------------------------------------------------------
#ifndef RESET_BUTTON_ENABLE
#define RESET_BUTTON_ENABLE 1
#endif
#ifndef RESET_BUTTON_PIN
#define RESET_BUTTON_PIN 28
#endif
// 0 = LOW で押下 (内部プルアップ + タクトスイッチをGNDへ落とす配線が既定)
#ifndef RESET_BUTTON_ACTIVE_LEVEL
#define RESET_BUTTON_ACTIVE_LEVEL 0
#endif
#ifndef RESET_BUTTON_DEBOUNCE_MS
#define RESET_BUTTON_DEBOUNCE_MS 20
#endif

#endif // CONFIG_H
