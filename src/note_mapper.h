#ifndef NOTE_MAPPER_H
#define NOTE_MAPPER_H

#include <stdint.h>

// 演奏に使う 21 キー (3 オクターブ × ダイアトニック 7 音)。
#define NOTE_MAP_KEY_COUNT 21

// note_to_key_index() が「出力なし」を返すときの値。
#define NOTE_MAP_NONE 0xFFu

// MIDI ノート番号 (0..127) を 0..20 のキースロット番号へ変換する純粋関数。
// config.h のポリシー (OCTAVE_OFFSET / OUT_OF_RANGE_POLICY /
// CHROMATIC_SNAP_POLICY) に従う。出力しない場合は NOTE_MAP_NONE を返す。
// 状態を持たないので Note On / Note Off どちらの経路からも同じ結果になる。
uint8_t note_to_key_index(uint8_t midi_note);

// キースロット番号 (0..20) に対応する USB HID キーコード
// (Keyboard/Keypad Usage Page 0x07)。範囲外なら 0。
uint8_t key_index_to_hid_code(uint8_t key_index);

// スロット番号 → HID キーコード表。NKRO レポートディスクリプタ生成と共有する。
// 添字がそのまま NKRO ビットマップのビット位置になる。
extern const uint8_t note_map_hid_codes[NOTE_MAP_KEY_COUNT];

#endif // NOTE_MAPPER_H
