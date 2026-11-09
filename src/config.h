#ifndef CONFIG_H
#define CONFIG_H

// ===========================================================================
//  原神 MIDI → CH9329 キーボード ガジェット 設定
//
//  構成:
//    [MIDI キーボード] --USB--> PIO-USB host (RP2350)
//    RP2350 native USB = USB-MIDI ガジェット (PC/DAW へ生 MIDI を出力)
//    RP2350 UART1       = CH9329 (9600 8N1) へ HID キーボードレポートを送信
//    CH9329 USB         = 操作対象 PC へ HID キーボードとして見える
//
//  すべて #ifndef ガード付きなので、CMake / コンパイル時 -D で上書きできる。
// ===========================================================================

// ---------------------------------------------------------------------------
//  移調 (キャリブレーション用)
//  入力 MIDI ノート番号に「加算する半音数」。12 = 1 オクターブ上。
//  実機で MIDI キーボードと原神の発音を鳴らし比べてズレていたら調整する。
// ---------------------------------------------------------------------------
#ifndef OCTAVE_OFFSET
#define OCTAVE_OFFSET 0
#endif

// ---------------------------------------------------------------------------
//  定義済みノート範囲 (固定)
//  低音域ド=48 … 高音域シ=83 の 3 オクターブ。 C4=MIDI60 基準。
// ---------------------------------------------------------------------------
#define NOTE_RANGE_LOW  48
#define NOTE_RANGE_HIGH 83

// ---------------------------------------------------------------------------
//  範囲外ノートのポリシー
//    IGNORE : MIDI 47 以下 / 84 以上は無視する (デフォルト)
//    WRAP   : 最寄りの定義済みオクターブへ ±12 ずつ折り返して丸め込む
// ---------------------------------------------------------------------------
#define OUT_OF_RANGE_IGNORE 0
#define OUT_OF_RANGE_WRAP   1
#ifndef OUT_OF_RANGE_POLICY
#define OUT_OF_RANGE_POLICY OUT_OF_RANGE_IGNORE
#endif

// ---------------------------------------------------------------------------
//  PIO-USB ホスト (MIDI 入力側) の GPIO
//  D+ を PIN_USB_HOST_DP、D- は必ずその隣 (DP+1) を使うこと。
//  新基板では D+ = GP12 / D- = GP13。
// ---------------------------------------------------------------------------
#ifndef PIN_USB_HOST_DP
#define PIN_USB_HOST_DP 12
#endif

// ---------------------------------------------------------------------------
//  MIDI 受信チャンネルのフィルタ
//  0 = 全チャンネル受け付け。 1..16 = そのチャンネルのみ。
// ---------------------------------------------------------------------------
#ifndef MIDI_CHANNEL_FILTER
#define MIDI_CHANNEL_FILTER 0
#endif

// ---------------------------------------------------------------------------
//  CH9329 (USB HID キーボードエミュレータ) 接続
//  CH9329 の出荷時設定は 9600 bps / 8N1 / プロトコルモード / Mode 0。
//  RP2350 の UART1 を CH9329 の RXD/TXD にクロス接続する。
//  TX=GP4, RX=GP5。デバッグログは UART0 (GP0/GP1) を使うため衝突しない。
// ---------------------------------------------------------------------------
#ifndef CH9329_KEYBOARD_ENABLE
#define CH9329_KEYBOARD_ENABLE 1
#endif
// 0 = uart0 / 1 = uart1
#ifndef CH9329_UART_INDEX
#define CH9329_UART_INDEX 1
#endif
#ifndef CH9329_UART_TX_PIN
#define CH9329_UART_TX_PIN 4
#endif
#ifndef CH9329_UART_RX_PIN
#define CH9329_UART_RX_PIN 5
#endif
#ifndef CH9329_UART_BAUD
#define CH9329_UART_BAUD 9600
#endif

// ---------------------------------------------------------------------------
//  MIDI ガジェット (native USB) へのパススルー
//  PIO-USB ホストが受けた生 MIDI バイト列を、そのまま USB-MIDI として
//  PC/DAW へ送る。core1 → core0 のキューを介する。
// ---------------------------------------------------------------------------
#ifndef MIDI_BRIDGE_QUEUE_DEPTH
#define MIDI_BRIDGE_QUEUE_DEPTH 8
#endif
#ifndef MIDI_BRIDGE_CHUNK_MAX
#define MIDI_BRIDGE_CHUNK_MAX 64
#endif
// tuh_midi_stream_read を 1 回に読み出す最大バイト数。
#ifndef MIDI_STREAM_CHUNK_MAX
#define MIDI_STREAM_CHUNK_MAX 64
#endif

// ---------------------------------------------------------------------------
//  原神モードスイッチ (任意)
//  ON (LOW) で CH9329 へのキーボード出力を停止する。USB-MIDI パススルーは
//  継続する。ミュート突入時は全キー解放レポートを CH9329 へ送る。
//  既定配線: GP28 ↔ GND をトグルスイッチで開閉 (内部プルアップ)。
// ---------------------------------------------------------------------------
#ifndef MUTE_SWITCH_ENABLE
#define MUTE_SWITCH_ENABLE 1
#endif
#ifndef MUTE_SWITCH_PIN
#define MUTE_SWITCH_PIN 28
#endif
// 0 = LOW でミュート (内部プルアップ + スイッチを GND へ閉じる配線が既定)
#ifndef MUTE_SWITCH_ACTIVE_LEVEL
#define MUTE_SWITCH_ACTIVE_LEVEL 0
#endif
#ifndef MUTE_SWITCH_DEBOUNCE_MS
#define MUTE_SWITCH_DEBOUNCE_MS 20
#endif

// ---------------------------------------------------------------------------
//  楽器モードスイッチ (任意)
//  ON にすると古びたライアー (スメール音階) 用のマッピングを使う。
//  既定配線: GP29 ↔ GND をトグルスイッチで開閉 (内部プルアップ)。
// ---------------------------------------------------------------------------
#ifndef LYRE_SWITCH_ENABLE
#define LYRE_SWITCH_ENABLE 1
#endif
#ifndef LYRE_SWITCH_PIN
#define LYRE_SWITCH_PIN 29
#endif
#ifndef LYRE_SWITCH_ACTIVE_LEVEL
#define LYRE_SWITCH_ACTIVE_LEVEL 0
#endif
#ifndef LYRE_SWITCH_DEBOUNCE_MS
#define LYRE_SWITCH_DEBOUNCE_MS MUTE_SWITCH_DEBOUNCE_MS
#endif

#endif // CONFIG_H
