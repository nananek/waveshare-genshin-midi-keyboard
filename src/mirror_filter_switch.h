#ifndef MIRROR_FILTER_SWITCH_H
#define MIRROR_FILTER_SWITCH_H

#include <stdbool.h>

// ミラーフィルタースイッチの入力管理。
// GPIO 初期化・デバウンスは core0 (メインループ) で行い、結果を volatile フラグで
// core1 (midi_mirror) に公開する。単語サイズの volatile 読み書きは RP2350 でアトミック。

void mirror_filter_switch_init(void);   // core0、メインループ開始前に 1 回
void mirror_filter_switch_poll(void);   // core0、各メインループイテレーションで 1 回
bool mirror_filter_switch_is_enabled(void); // コア跨ぎで読む (true = フィルター ON)

#endif // MIRROR_FILTER_SWITCH_H