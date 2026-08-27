#ifndef MIDI_BRIDGE_H
#define MIDI_BRIDGE_H

#include <stdint.h>

// core1→core0 の RAW MIDI バイト転送 + USB-MIDI 送信 + RESET/SUSTAIN
// core1 文脈 (tuh_midi_rx_cb) と core0 文脈 (main loop) で分けて呼ぶ。

void midi_bridge_init(void);
void midi_bridge_reset(void);

// core1 から呼ぶ (tuh_midi_rx_cb 文脈)
void midi_bridge_push(const uint8_t *data, uint32_t len);

// core0 から呼ぶ (main loop, tud_task() 後)
void midi_bridge_task(void);

// RESET / SUSTAIN 用 — core0 から呼ぶ (tud_midi_packet_write で送る)
void midi_bridge_send_realtime(uint8_t status);
void midi_bridge_send_cc(uint8_t ch, uint8_t cc, uint8_t val);

#endif // MIDI_BRIDGE_H
