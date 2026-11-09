#ifndef SPARE_SWITCH_H
#define SPARE_SWITCH_H

#include <stdbool.h>

// 予備スイッチ入力 (ハード依存層)。
// 旧ボード2のサステインスイッチを、PIO-USB MIDI-HID ガジェット側の
// 予備入力へ移したもの。既定では状態を読むだけで、MIDI へは何も送らない。
// SPARE_SWITCH_CC を 0..127 に設定すると、エッジでその CC を CH345 へ送る。

typedef enum {
    SPARE_SWITCH_NO_CHANGE = 0,
    SPARE_SWITCH_ENTER, // アクティブになった
    SPARE_SWITCH_EXIT,  // 非アクティブになった
} spare_switch_edge_t;

void spare_switch_init(void);
spare_switch_edge_t spare_switch_poll(void);
bool spare_switch_is_active(void);

#endif // SPARE_SWITCH_H
