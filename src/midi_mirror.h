#ifndef MIDI_MIRROR_H
#define MIDI_MIRROR_H

#include <stdint.h>

// RAW MIDI バイト列を UART へミラー出力する (core1 の tuh_midi コールバック文脈から呼ぶ)。
// 設定は config.h の MIDI_UART_MIRROR_* 。無効時は no-op になり、コードも
// コンパイルアウトされる。

void midi_mirror_init(void);

// data[0..len) をミラー UART へ書き出す。ブロッキング送信。
// フィルタースイッチ (mirror_filter_switch) が OFF ならバイト同一のパススルー、
// ON なら原神鍵盤の Note On/Off のみへ絞り込んで明示ステータス 3 バイトへ
// 再構成した内容を書き出す (data とは長さ・内容とも一致しない)。
void midi_mirror_send(const uint8_t *data, uint32_t len);

// 接続デバイス変更時 (リマウント) に内部状態 (ランニングステータス等) をリセットする。
void midi_mirror_reset(void);

#endif // MIDI_MIRROR_H
