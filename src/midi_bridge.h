#ifndef MIDI_BRIDGE_H
#define MIDI_BRIDGE_H

#include <stdint.h>

// PIO-USB ホスト (core1) が受けた生 MIDI バイト列を、native USB の
// USB-MIDI デバイス (core0) へパススルーする。
//
// core1 から midi_bridge_send()、core0 から midi_bridge_task() を呼ぶ。
// TinyUSB の MIDI ストリーム状態機械は core0 だけが触る。

void midi_bridge_init(void);

// core1: 受信バイト列をキューへ積む。溢れた分は捨てる (演奏用途で許容)。
void midi_bridge_send(const uint8_t *bytes, uint32_t length);

// core0: USB-MIDI の TX FIFO へ送れる分だけ送る。毎ループ呼ぶ。
void midi_bridge_task(void);

#endif // MIDI_BRIDGE_H
