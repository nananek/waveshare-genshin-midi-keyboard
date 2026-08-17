#ifndef MIRROR_FILTER_SWITCH_H
#define MIRROR_FILTER_SWITCH_H

#include <stdbool.h>

// ミラーフィルタースイッチの入力管理。
// GPIO 初期化・デバウンスは core0 (メインループ) で行い、結果を volatile フラグで
// core1 (midi_mirror) に公開する。共有フラグは bool (1 バイト) なので RP2350 上の
// 単純な読み書きはアトミック (マルチバイトの値をこの手法で共有する場合は別途排他が要る)。

void mirror_filter_switch_init(void);   // core0、メインループ開始前に 1 回
void mirror_filter_switch_poll(void);   // core0、各メインループイテレーションで 1 回
bool mirror_filter_switch_is_enabled(void); // コア跨ぎで読む (true = フィルター ON)

#endif // MIRROR_FILTER_SWITCH_H