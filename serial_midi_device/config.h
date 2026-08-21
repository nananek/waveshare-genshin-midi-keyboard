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

// ---------------------------------------------------------------------------
//  サステインスライドスイッチ (issue #8)
//  ON/OFFが変化した瞬間にCC64(サステインペダル)=127/0を1回だけ送信する
//  (継続送出はしない。受信側がサステイン状態を保持するMIDI仕様のため)。
//  ボード1のLYRE_SWITCH_PIN(GP29、src/config.h)と同一物理ピン番号に揃えている。
// ---------------------------------------------------------------------------
#ifndef SUSTAIN_SWITCH_ENABLE
#define SUSTAIN_SWITCH_ENABLE 1
#endif
#ifndef SUSTAIN_SWITCH_PIN
#define SUSTAIN_SWITCH_PIN 29
#endif
// 0 = LOW でサステインON (内部プルアップ + スイッチをGNDへ閉じる配線が既定)
#ifndef SUSTAIN_SWITCH_ACTIVE_LEVEL
#define SUSTAIN_SWITCH_ACTIVE_LEVEL 0
#endif
// デバウンス時間(ms)。既定値はリセットボタンと共有 (個別に-Dで上書き可)。
#ifndef SUSTAIN_SWITCH_DEBOUNCE_MS
#define SUSTAIN_SWITCH_DEBOUNCE_MS RESET_BUTTON_DEBOUNCE_MS
#endif
// CC64を送信するMIDIチャンネル (0-indexed。既定 0 = MIDIチャンネル1)
#ifndef SUSTAIN_MIDI_CHANNEL
#define SUSTAIN_MIDI_CHANNEL 0
#endif

#endif // CONFIG_H
