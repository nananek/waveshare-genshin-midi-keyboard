#ifndef CONFIG_H
#define CONFIG_H

// ===========================================================================
//  諧律のチェンバロ MIDI→HID コンバータ 設定
//  指示書 第4章「設定可能パラメータ」の一元管理場所。
//  すべて #ifndef ガード付きなので、CMake / コンパイル時 -D で上書きできる。
// ===========================================================================

// ---------------------------------------------------------------------------
//  移調 (キャリブレーション用)
//  入力 MIDI ノート番号に「加算する半音数」。
//  実機で MIDI キーボードと原神の発音を鳴らし比べてズレていたら、この値で
//  全体を移調して補正する。 12 = 1 オクターブ上、 -12 = 1 オクターブ下。
//  (名前は OCTAVE_OFFSET だが単位は半音。細かく合わせられるようにしている)
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
//  範囲外ノートのポリシー (指示書 3.2)
//    IGNORE : MIDI 47 以下 / 84 以上は無視して HID 出力しない (デフォルト)
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
//  Waveshare RP2350-USB-A の USB-A ポートは D+=GP12 / D-=GP13 に配線されている。
//  ※ 当該ボードはホスト動作のため D+ プルアップ抵抗 R13(1.5kΩ) の除去が必要
//    (詳細は README のハードウェア改造を参照)。
//  自作配線や他ボードを使う場合はここを合わせること。
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
//  RAW MIDI の UART ミラー出力
//  MIDI キーボードから受信したバイト列をそのまま別 UART へ 31250 baud で
//  ミラー出力する (Note/CC/ピッチベンド/SysEx 等すべて、パース・変換は通さない)。
//  デバッグログ (stdio-uart) は UART0 (GP0/GP1) を占有しているため、
//  既定で UART1 (TX=GP4) を使う。シリアル→USB-MIDI ブリッジ (ボード2) は
//  RX=GP5 で受ける。0 にするとミラーはコンパイルアウトされる。
// ---------------------------------------------------------------------------
#ifndef MIDI_UART_MIRROR_ENABLE
#define MIDI_UART_MIRROR_ENABLE 1
#endif
#ifndef MIDI_UART_MIRROR_BAUD
#define MIDI_UART_MIRROR_BAUD 31250
#endif
// 0 = uart0 / 1 = uart1 (デバッグ uart0 と分離するため 1 が既定)
#ifndef MIDI_UART_MIRROR_UART
#define MIDI_UART_MIRROR_UART 1
#endif
#ifndef MIDI_UART_MIRROR_TX_PIN
#define MIDI_UART_MIRROR_TX_PIN 4
#endif
// tuh_midi_stream_read を 1 回に読み出す最大バイト数 (midi_host.c の受信バッファと
// midi_mirror.c のフィルター出力バッファのサイズ計算が共有する)。
#ifndef MIDI_STREAM_CHUNK_MAX
#define MIDI_STREAM_CHUNK_MAX 64
#endif

// ---------------------------------------------------------------------------
//  TPS2553 downstream USB-A power control (board 1)
//  GP9=EN, GP10=FAULT_N.  EN has an external 100k pull-down and therefore
//  remains OFF during reset/boot before firmware takes ownership.
// ---------------------------------------------------------------------------
#ifndef PIN_USB_POWER_EN
#define PIN_USB_POWER_EN 9
#endif
#ifndef PIN_USB_POWER_FAULT_N
#define PIN_USB_POWER_FAULT_N 10
#endif
// Waveshare RP2350-Zero schematic: onboard WS2812B DIN is GPIO16.
#ifndef PIN_STATUS_WS2812
#define PIN_STATUS_WS2812 16
#endif

// ---------------------------------------------------------------------------
//  原神モードスイッチ (ボード1: MIDI→HID 変換板)
//  スイッチを ON (ミュート) にすると HID キーボード出力を無効化し、
//  UART ミラーは完全パススルーにする。OFF (原神演奏中) では HID を有効化し、
//  UART ミラーを原神鍵盤ノートだけに絞る。
//  既定配線: GP28 ↔ GND をトグルスイッチで開閉 (内部プルアップを常時有効化)。
//  0 = ピン LOW でミュート (スイッチ閉) / 1 = ピン HIGH でミュート。
//  0 にすると原神モードスイッチ機能はコンパイルアウトされる。
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
// デバウンス時間 (ms)。この時間連続して同じレベルを観測したら状態を確定する。
#ifndef MUTE_SWITCH_DEBOUNCE_MS
#define MUTE_SWITCH_DEBOUNCE_MS 20
#endif

// ---------------------------------------------------------------------------
//  楽器モードスイッチ (ボード1)
//  ON にすると古びたライアー (スメール音階) 用のマッピングを使う。
//  OFF は通常のチェンバロ／ライアー用マッピング。
//  既定配線: GP29 ↔ GND をトグルスイッチで開閉 (内部プルアップを常時有効化)。
//  0 = ピン LOW でスメール音階 ON / 1 = ピン HIGH でスメール音階 ON。
//  0 にすると通常マッピングに固定される。
// ---------------------------------------------------------------------------
#ifndef LYRE_SWITCH_ENABLE
#define LYRE_SWITCH_ENABLE 1
#endif
#ifndef LYRE_SWITCH_PIN
#define LYRE_SWITCH_PIN 29
#endif
// 0 = LOW でスメール音階 ON (内部プルアップ + スイッチを GND へ閉じる配線が既定)
#ifndef LYRE_SWITCH_ACTIVE_LEVEL
#define LYRE_SWITCH_ACTIVE_LEVEL 0
#endif
// デバウンス時間 (ms)。既定値は原神モードスイッチと共有 (個別に -D で上書き可)。
#ifndef LYRE_SWITCH_DEBOUNCE_MS
#define LYRE_SWITCH_DEBOUNCE_MS MUTE_SWITCH_DEBOUNCE_MS
#endif

#endif // CONFIG_H
