#ifndef MIDI_UART_H
#define MIDI_UART_H

#include <stdint.h>

// CH345 (UART ↔ USB-MIDI) への RAW MIDI 出力。
// PIO-USB ホストが受けた MIDI 1.0 バイト列を、そのまま 31250 bps 8N1 で送る。
// CH345 側が USB-MIDI クラスとして PC/DAW に見える。
//
// core1 (tuh_task のコールバック文脈) からのみ呼ぶこと。

void midi_uart_init(void);
void midi_uart_send(const uint8_t *bytes, uint32_t length);

// 予備スイッチなどから MIDI CC を 1 個送る。UART の排他は内部で行う。
void midi_uart_send_cc(uint8_t channel, uint8_t cc, uint8_t value);

#endif // MIDI_UART_H
