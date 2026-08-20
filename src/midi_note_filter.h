#ifndef MIDI_NOTE_FILTER_H
#define MIDI_NOTE_FILTER_H

#include <stdint.h>
#include <stdbool.h>

// ミラー出力用の「原神鍵盤のみ」フィルター (純粋 C、ホスト単体テスト対象)。
// MIDI 1.0 バイト列を入力し、通過させる Note On/Off を明示ステータス 3 バイト
// {status, note, velocity} で出力バッファへ再構成する。
// ランニングステータス入力を解釈し、SysEx / システムコモン / リアルタイム /
// 非 Note チャンネルメッセージは破棄する。入力がチャンク境界で分断されても
// 状態を保持するので連続的に呼べる。

typedef struct {
    uint8_t phase;          // 0=READY / 1=HAVE_NOTE / 2=DROPPING / 3=SYSEX
    uint8_t running_status; // 直前の Note Off/On ステータス (0 = なし)
    uint8_t pending_note;   // HAVE_NOTE 時のノート番号
    uint8_t drop_count;     // DROPPING 時に破棄する残データバイト数
    bool sumeru_mode;
} midi_note_filter_t;

void midi_note_filter_init(midi_note_filter_t *f);
void midi_note_filter_set_sumeru_mode(midi_note_filter_t *f, bool enabled);

// in[0..len) を処理し、通過分を out に追記する。ランニングステータスで圧縮された
// 2 バイトのノートは明示ステータス 3 バイトへ展開されるため、出力は入力長の
// 最大 1.5 倍 (+ 前回呼び出しから持ち越した note 1 個分) まで増え得る。
// out_cap が不足する場合、超過分は破棄される (呼び出し側が十分な容量を
// 確保すること)。返り値は out に書いたバイト数。
uint32_t midi_note_filter_process(midi_note_filter_t *f,
                                  const uint8_t *in, uint32_t len,
                                  uint8_t *out, uint32_t out_cap);

// このノートが原神で使える鍵盤に対応するか (note_mapper の判定をそのまま使用)。
bool midi_note_filter_is_diatonic(uint8_t note);
bool midi_note_filter_is_diatonic_mode(uint8_t note, bool sumeru_mode);

// f が現在メッセージ境界 (次に来るバイトが新しい MIDI メッセージの先頭になり得る
// 状態) にあるか。ステータス+note を受領済みで velocity 待ちの間や SysEx/ドロップ
// 中は false。呼び出し側がフィルターの有効/無効をメッセージ途中で切り替えて
// 出力を破損させないためのタイミング判定に使う。
bool midi_note_filter_is_ready(const midi_note_filter_t *f);

#endif // MIDI_NOTE_FILTER_H
