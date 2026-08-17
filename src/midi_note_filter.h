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
} midi_note_filter_t;

void midi_note_filter_init(midi_note_filter_t *f);

// in[0..len) を処理し、通過分を out に追記する。out には len バイト分の容量を
// 渡すこと (出力は入力長を超えない)。返り値は out に書いたバイト数。
uint32_t midi_note_filter_process(midi_note_filter_t *f,
                                  const uint8_t *in, uint32_t len,
                                  uint8_t *out, uint32_t out_cap);

// このノートが原神で使える鍵盤に対応するか (note_mapper の判定をそのまま使用)。
bool midi_note_filter_is_diatonic(uint8_t note);

#endif // MIDI_NOTE_FILTER_H