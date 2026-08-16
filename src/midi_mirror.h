#ifndef MIDI_MIRROR_H
#define MIDI_MIRROR_H

#include <stdint.h>

// RAW MIDI バイト列を UART へミラー出力する (core1 の tuh_midi コールバック文脈から呼ぶ)。
// 設定は config.h の MIDI_UART_MIRROR_* 。無効時は no-op になり、コードも
// コンパイルアウトされる。

void midi_mirror_init(void);

// data[0..len) をそのままミラー UART へ書き出す。ブロッキング送信。
void midi_mirror_send(const uint8_t *data, uint32_t len);

#endif // MIDI_MIRROR_H