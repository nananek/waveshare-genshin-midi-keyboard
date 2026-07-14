#ifndef MIDI_PARSE_H
#define MIDI_PARSE_H

#include <stdint.h>
#include <stdbool.h>

// USB-MIDI から取り出した生の MIDI バイトストリームを解析し、
// Note On / Note Off イベントだけをコールバックで通知する状態機械。
// ランニングステータス・リアルタイムメッセージ混在・SysEx をまたいでも
// 破綻しないよう設計している。状態は構造体に閉じているので単体テスト可能。

typedef struct {
    uint8_t status;         // 現在解釈中のステータス (channel voice / system common)
    uint8_t running_status; // channel voice のみ保持 (ランニングステータス用)
    uint8_t data[2];        // 収集済みデータバイト
    uint8_t data_count;     // 収集済み数
    uint8_t expected;       // 現ステータスが必要とするデータバイト数
    bool    in_sysex;       // SysEx (F0..F7) の途中
} midi_parser_t;

// Note イベントコールバック。
//   on == true  → Note On  (velocity > 0)
//   on == false → Note Off (0x8n、または 0x9n + velocity 0)
// channel は 0..15。
typedef void (*midi_note_cb_t)(uint8_t channel, uint8_t note, bool on, void *ctx);

void midi_parser_init(midi_parser_t *p);

// bytes[0..n) を流し込む。Note On/Off を見つけるたび cb を呼ぶ。
void midi_parser_push(midi_parser_t *p, const uint8_t *bytes, uint32_t n,
                      midi_note_cb_t cb, void *ctx);

#endif // MIDI_PARSE_H
