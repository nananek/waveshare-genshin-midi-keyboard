#include "midi_note_filter.h"
#include "note_mapper.h"

// フェーズ (内部状態)
#define PH_READY     0 // ステータス待ち / ランニングステータス継続の note 待ち
#define PH_HAVE_NOTE 1 // note を受領済み、velocity 待ち
#define PH_DROPPING  2 // 非 Note メッセージのデータを drop_count バイト破棄中
#define PH_SYSEX     3 // SysEx 中 (F7 まで破棄)

bool midi_note_filter_is_diatonic(uint8_t note) {
    return note_to_key_index(note) != NOTE_MAP_NONE;
}

void midi_note_filter_init(midi_note_filter_t *f) {
    f->phase = PH_READY;
    f->running_status = 0;
    f->pending_note = 0;
    f->drop_count = 0;
}

static uint32_t emit_note(midi_note_filter_t *f, uint8_t status,
                          uint8_t note, uint8_t vel, uint8_t *out, uint32_t cap) {
    (void)f;
    if (!midi_note_filter_is_diatonic(note)) {
        return 0; // 原神鍵盤以外のノートは破棄 (Note Off も同様に破棄される)
    }
    if (cap < 3) {
        return 0; // 出力バッファ不足 (入力長≦バッファなので実際は起きない)
    }
    out[0] = status;
    out[1] = note;
    out[2] = vel;
    return 3;
}

uint32_t midi_note_filter_process(midi_note_filter_t *f,
                                  const uint8_t *in, uint32_t len,
                                  uint8_t *out, uint32_t out_cap) {
    uint32_t o = 0;
    for (uint32_t i = 0; i < len; i++) {
        uint8_t b = in[i];
        if (b >= 0xF8) {
            // リアルタイム: どこにでも挿入される。破棄のみで状態は変えない。
            continue;
        }
        if (b >= 0xF0) {
            // システムコモン / SysEx (F0..F7)。F7 は SysEx 終了も兼ねる。
            if (b == 0xF0) {
                f->phase = PH_SYSEX;
                f->running_status = 0;
            } else if (b == 0xF7) {
                f->phase = PH_READY;
            } else {
                f->running_status = 0;
                f->phase = PH_DROPPING;
                f->drop_count = (b == 0xF2) ? 2 : (b == 0xF1 || b == 0xF3) ? 1 : 0;
                if (f->drop_count == 0) {
                    f->phase = PH_READY;
                }
            }
            continue;
        }
        if (b >= 0x80) {
            // チャンネルメッセージ (Note Off 0x8n / Note On 0x9n のみ状態に保持)
            if (b <= 0x9F) {
                f->running_status = b;
                f->phase = PH_READY; // 次のデータバイトは note (ランニングステータス継続)
            } else {
                f->running_status = 0; // CC/PC/PB はランニングステータスを解除
                f->phase = PH_DROPPING;
                f->drop_count = (b >= 0xC0 && b <= 0xDF) ? 1 : 2;
            }
            continue;
        }
        // データバイト (b < 0x80)
        switch (f->phase) {
        case PH_READY:
            // ランニングステータス継続ならこのバイトは note 番号
            if (f->running_status != 0) {
                f->pending_note = b;
                f->phase = PH_HAVE_NOTE; // velocity 待ち
            }
            break; // ステータス無しの迷子データは破棄
        case PH_HAVE_NOTE: {
            uint8_t st = f->running_status;
            f->phase = PH_READY; // 次はランニングステータス継続で note 待ち
            o += emit_note(f, st, f->pending_note, b, out + o, out_cap - o);
            break;
        }
        case PH_DROPPING:
            if (f->drop_count > 0) {
                f->drop_count--;
                if (f->drop_count == 0) {
                    f->phase = PH_READY;
                }
            }
            break;
        default: // PH_SYSEX
            break; // 破棄
        }
    }
    return o;
}