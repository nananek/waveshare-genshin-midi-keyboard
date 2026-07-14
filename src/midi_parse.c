#include "midi_parse.h"

// ステータスバイトが必要とするデータバイト数。
static uint8_t expected_data_bytes(uint8_t status) {
    if (status < 0xF0) {
        switch (status & 0xF0) {
            case 0x80: // Note Off
            case 0x90: // Note On
            case 0xA0: // Poly Aftertouch
            case 0xB0: // Control Change
            case 0xE0: // Pitch Bend
                return 2;
            case 0xC0: // Program Change
            case 0xD0: // Channel Aftertouch
                return 1;
            default:
                return 0;
        }
    }
    // System Common
    switch (status) {
        case 0xF2: return 2; // Song Position Pointer
        case 0xF1:           // MTC Quarter Frame
        case 0xF3: return 1; // Song Select
        default:  return 0;  // F4..F6 等
    }
}

void midi_parser_init(midi_parser_t *p) {
    p->status = 0;
    p->running_status = 0;
    p->data[0] = 0;
    p->data[1] = 0;
    p->data_count = 0;
    p->expected = 0;
    p->in_sysex = false;
}

static void emit(midi_parser_t *p, midi_note_cb_t cb, void *ctx) {
    uint8_t type = p->status & 0xF0;
    uint8_t channel = p->status & 0x0F;
    if (type == 0x90) {          // Note On (velocity 0 は Note Off 扱い)
        cb(channel, p->data[0], p->data[1] > 0, ctx);
    } else if (type == 0x80) {   // Note Off
        cb(channel, p->data[0], false, ctx);
    }
    // その他の channel voice / system common は無視
}

void midi_parser_push(midi_parser_t *p, const uint8_t *bytes, uint32_t n,
                      midi_note_cb_t cb, void *ctx) {
    for (uint32_t i = 0; i < n; i++) {
        uint8_t b = bytes[i];

        // System Real-Time (0xF8..0xFF): どこに挟まってもよく、状態を乱さない。
        if (b >= 0xF8) {
            continue;
        }

        if (b >= 0x80) {
            // 何らかのステータスバイト
            if (b == 0xF0) {            // SysEx 開始
                p->in_sysex = true;
                p->running_status = 0;
                p->status = 0;
                p->data_count = 0;
                continue;
            }
            if (b == 0xF7) {            // SysEx 終了 (EOX)
                p->in_sysex = false;
                p->status = 0;
                p->data_count = 0;
                continue;
            }
            p->in_sysex = false;
            p->status = b;
            p->data_count = 0;
            p->expected = expected_data_bytes(b);
            // channel voice のみランニングステータスを保持。system common は解除。
            p->running_status = (b < 0xF0) ? b : 0;
            if (p->expected == 0) {
                p->status = 0; // データ不要なメッセージ (F6 等) は即完了
            }
            continue;
        }

        // ここからデータバイト
        if (p->in_sysex) {
            continue; // SysEx ペイロードは読み飛ばす
        }
        if (p->status == 0) {
            // ステータス未確定。channel voice のランニングステータスがあれば復帰。
            if (p->running_status != 0) {
                p->status = p->running_status;
                p->expected = expected_data_bytes(p->status);
                p->data_count = 0;
            } else {
                continue; // 迷子のデータバイトは捨てる
            }
        }

        if (p->data_count < 2) {
            p->data[p->data_count] = b;
        }
        p->data_count++;

        if (p->data_count >= p->expected) {
            emit(p, cb, ctx);
            p->data_count = 0;
            // channel voice はランニングステータスで次メッセージへ。
            p->status = (p->running_status != 0) ? p->running_status : 0;
        }
    }
}
