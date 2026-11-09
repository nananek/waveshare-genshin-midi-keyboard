#include "midi_bridge.h"

#include <string.h>

#include "pico/util/queue.h"
#include "tusb.h"

#include "config.h"

typedef struct {
    uint16_t length;
    uint8_t data[MIDI_BRIDGE_CHUNK_MAX];
} midi_bridge_event_t;

static queue_t s_queue;
static bool s_initialized;

// core0 専用。tud_midi_stream_write の部分書き込みを持ち越す。
static uint8_t s_carry[256];
static uint32_t s_carry_length;

void midi_bridge_init(void) {
    if (s_initialized) {
        return;
    }
    queue_init(&s_queue, sizeof(midi_bridge_event_t), MIDI_BRIDGE_QUEUE_DEPTH);
    s_carry_length = 0;
    s_initialized = true;
}

void midi_bridge_send(const uint8_t *bytes, uint32_t length) {
    if (!s_initialized || bytes == NULL || length == 0) {
        return;
    }
    while (length > 0) {
        uint32_t chunk = length;
        if (chunk > MIDI_BRIDGE_CHUNK_MAX) {
            chunk = MIDI_BRIDGE_CHUNK_MAX;
        }
        midi_bridge_event_t event;
        event.length = (uint16_t)chunk;
        memcpy(event.data, bytes, chunk);
        // キューが満杯なら、このチャンクは捨てる。MIDI は絶対時刻では
        // ないため、遅延して溜めるより落とした方が破綻しにくい。
        if (!queue_try_add(&s_queue, &event)) {
            return;
        }
        bytes += chunk;
        length -= chunk;
    }
}

static void flush_carry(void) {
    while (s_carry_length > 0) {
        uint32_t written = tud_midi_stream_write(0, s_carry, s_carry_length);
        if (written == 0) {
            return;
        }
        memmove(s_carry, s_carry + written, s_carry_length - written);
        s_carry_length -= written;
    }
}

void midi_bridge_task(void) {
    if (!s_initialized) {
        return;
    }
    // PC が USB-MIDI を列挙していない間は溜めずに捨てる。再接続時に
    // 古いノートが一気に流れて固着するのを防ぐ。
    if (!tud_mounted()) {
        s_carry_length = 0;
        midi_bridge_event_t discard;
        while (queue_try_remove(&s_queue, &discard)) {
        }
        return;
    }

    flush_carry();

    midi_bridge_event_t event;
    while (queue_try_remove(&s_queue, &event)) {
        if (event.length > sizeof(s_carry) - s_carry_length) {
            continue; // 持ち越しが一杯: このチャンクを捨てる
        }
        memcpy(s_carry + s_carry_length, event.data, event.length);
        s_carry_length += event.length;
        flush_carry();
        if (s_carry_length > 0) {
            return; // FIFO 満杯。次回続きを送る。
        }
    }
}
