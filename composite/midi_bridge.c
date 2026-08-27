#include "midi_bridge.h"
#include "midi_note_filter.h"
#include "hid_mute.h"
#include "mirror_filter_switch.h"
#include "config.h"

#include "tusb.h"
#include "pico/util/queue.h"
#include <string.h>

// ===========================================================================
//  core1→core0 の RAW MIDI 転送ブリッジ
//  - core1 の tuh_midi_rx_cb で受けた生 MIDI バイト列を queue 経由で core0 へ
//  - core0 で tud_midi_stream_write / tud_midi_packet_write へ流す
//  - midi_mirror.c と同等のフィルタリング (原神鍵盤のみ) をメッセージ境界で切替
//  - FIFO 満杯時は carry に持ち越し (serial_midi_device と同ポリシー)
// ===========================================================================

// MIDI_STREAM_CHUNK_MAX (config.h) + ランニングステータス展開を見込んだ出力上限
#define MIDI_BRIDGE_FILTERED_BUF_LEN ((MIDI_STREAM_CHUNK_MAX * 3) / 2 + 3)

#define MIDI_BRIDGE_QUEUE_DEPTH 16
#define MIDI_BRIDGE_CABLE_NUM 0

typedef struct {
    uint8_t len;
    uint8_t data[MIDI_STREAM_CHUNK_MAX * 2]; // filtered で最大 1.5倍なので余裕を持たせる
} midi_bridge_chunk_t;

static queue_t s_midi_queue;
static bool s_queue_inited;
static uint8_t s_carry[MIDI_STREAM_CHUNK_MAX * 2];
static uint32_t s_carry_len;
static midi_note_filter_t s_filter;
static bool s_filter_active;

static void sync_filter_mode(void) {
    if (midi_note_filter_is_ready(&s_filter)) {
        s_filter_active = hid_mute_should_filter_mirror();
        midi_note_filter_set_sumeru_mode(&s_filter, mirror_filter_switch_is_enabled());
    }
}

void midi_bridge_init(void) {
    if (!s_queue_inited) {
        queue_init(&s_midi_queue, sizeof(midi_bridge_chunk_t), MIDI_BRIDGE_QUEUE_DEPTH);
        s_queue_inited = true;
    }
    s_carry_len = 0;
    midi_note_filter_init(&s_filter);
    s_filter_active = hid_mute_should_filter_mirror();
    midi_note_filter_set_sumeru_mode(&s_filter, mirror_filter_switch_is_enabled());
}

void midi_bridge_reset(void) {
    midi_note_filter_init(&s_filter);
    s_filter_active = hid_mute_should_filter_mirror();
    midi_note_filter_set_sumeru_mode(&s_filter, mirror_filter_switch_is_enabled());
}

void midi_bridge_push(const uint8_t *data, uint32_t len) {
    if (len == 0) {
        return;
    }
    sync_filter_mode();

    // パススルー中も内部状態を追従させるため常に process を通す
    uint8_t filtered[MIDI_BRIDGE_FILTERED_BUF_LEN];
    uint32_t n = midi_note_filter_process(&s_filter, data, len, filtered, sizeof(filtered));

    midi_bridge_chunk_t chunk;
    if (s_filter_active) {
        if (n == 0) {
            return; // フィルタで全て破棄された
        }
        // filtered は n バイト。64B を超える場合は分割して queue に積む
        uint32_t offset = 0;
        while (offset < n) {
            uint32_t frag = n - offset;
            if (frag > sizeof(chunk.data)) {
                frag = sizeof(chunk.data);
            }
            chunk.len = (uint8_t)frag;
            memcpy(chunk.data, filtered + offset, frag);
            queue_try_add(&s_midi_queue, &chunk);
            offset += frag;
        }
    } else {
        // 完全パススルー — 入力をそのまま (分割が必要なら分割)
        uint32_t offset = 0;
        while (offset < len) {
            uint32_t frag = len - offset;
            if (frag > sizeof(chunk.data)) {
                frag = sizeof(chunk.data);
            }
            chunk.len = (uint8_t)frag;
            memcpy(chunk.data, data + offset, frag);
            queue_try_add(&s_midi_queue, &chunk);
            offset += frag;
        }
    }
}

static void flush_carry(void) {
    while (s_carry_len > 0) {
        uint32_t n = tud_midi_stream_write(0, s_carry, s_carry_len);
        if (n == 0) {
            return;
        }
        memmove(s_carry, s_carry + n, s_carry_len - n);
        s_carry_len -= n;
    }
}

void midi_bridge_task(void) {
    flush_carry();

    midi_bridge_chunk_t chunk;
    while (queue_try_remove(&s_midi_queue, &chunk)) {
        if (chunk.len == 0) {
            continue;
        }
        uint32_t n = tud_midi_stream_write(0, chunk.data, chunk.len);
        if (n < chunk.len) {
            // FIFO 満杯で部分書き込み — 残りを carry に保持
            if (s_carry_len == 0) {
                uint32_t remain = chunk.len - n;
                if (remain <= sizeof(s_carry)) {
                    memcpy(s_carry, chunk.data + n, remain);
                    s_carry_len = remain;
                }
            }
            // carry が既に残っている間に新データが来た場合は捨てる (serial と同ポリシー)
            break;
        }
    }
}

void midi_bridge_send_realtime(uint8_t status) {
    uint8_t packet[4] = {
        (uint8_t)((MIDI_BRIDGE_CABLE_NUM << 4) | MIDI_CIN_SYSEX_END_1BYTE),
        status, 0, 0,
    };
    tud_midi_packet_write(packet);
}

void midi_bridge_send_cc(uint8_t ch, uint8_t cc, uint8_t val) {
    uint8_t packet[4] = {
        (uint8_t)((MIDI_BRIDGE_CABLE_NUM << 4) | MIDI_CIN_CONTROL_CHANGE),
        (uint8_t)(0xB0 | (ch & 0x0F)),
        cc,
        val,
    };
    tud_midi_packet_write(packet);
}
