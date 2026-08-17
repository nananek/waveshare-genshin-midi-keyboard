#ifndef DEBOUNCED_SWITCH_H
#define DEBOUNCED_SWITCH_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/time.h"

// 汎用デバウンス付き GPIO トグルスイッチ入力 (ハード依存層)。
// hid_mute / mirror_filter_switch が共有する実装。時間ベース (連続観測) デバウンス。
// 内部プルは active_level に応じて自動選択する (開放時は必ず非アクティブになる)。

typedef struct {
    uint8_t pin;
    bool active_level;    // gpio_get(pin) がこの値ならアクティブ
    uint32_t debounce_ms;
    bool committed;        // 確定済みのアクティブ状態
    bool candidate;         // デバウンス対象の候補
    bool sampled;            // 前回の生読み取り (チャタリング検知用)
    absolute_time_t candidate_since;
} debounced_switch_t;

typedef enum {
    DEBOUNCED_SWITCH_NO_CHANGE = 0,
    DEBOUNCED_SWITCH_ENTER, // 非アクティブ → アクティブ に確定
    DEBOUNCED_SWITCH_EXIT,  // アクティブ → 非アクティブ に確定
} debounced_switch_edge_t;

// GPIO 初期化 (入力 + active_level に応じた内部プル) + 現在値で状態を確定初期化する。
void debounced_switch_init(debounced_switch_t *s, uint8_t pin, bool active_level,
                            uint32_t debounce_ms);

// 呼び出し元のポーリングループから毎回呼ぶ。状態遷移が確定したらエッジを返す。
debounced_switch_edge_t debounced_switch_poll(debounced_switch_t *s);

// 現在のデバウンス済み状態 (true = アクティブ)。
bool debounced_switch_is_active(const debounced_switch_t *s);

#endif // DEBOUNCED_SWITCH_H
