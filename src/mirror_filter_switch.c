#include "mirror_filter_switch.h"
#include "debounced_switch.h"
#include "config.h"

// ===========================================================================
//  ミラーフィルタースイッチ入力 (ハード依存層)。デバウンス本体は debounced_switch を使う。
//  既定配線: GP29 ↔ GND のトグルスイッチ。閉じる (LOW) = アクティブ = フィルター ON。
//  デバウンス確定値を volatile 共有フラグに書き、core1 (midi_mirror) が読む。
//  無効時 (MIRROR_FILTER_SWITCH_ENABLE=0) は no-op + 常にパススルー。
// ===========================================================================

#if MIRROR_FILTER_SWITCH_ENABLE

static debounced_switch_t s_sw;
static volatile bool s_enabled; // core0 が書き、core1 が読む共有フラグ (bool は 1 バイトでアトミック)

void mirror_filter_switch_init(void) {
    debounced_switch_init(&s_sw, MIRROR_FILTER_SWITCH_PIN,
                          (bool)MIRROR_FILTER_SWITCH_ACTIVE_LEVEL,
                          (uint32_t)MIRROR_FILTER_SWITCH_DEBOUNCE_MS);
    s_enabled = debounced_switch_is_active(&s_sw);
}

void mirror_filter_switch_poll(void) {
    if (debounced_switch_poll(&s_sw) != DEBOUNCED_SWITCH_NO_CHANGE) {
        s_enabled = debounced_switch_is_active(&s_sw); // デバウンス確定時に共有フラグへ反映
    }
}

bool mirror_filter_switch_is_enabled(void) {
    return s_enabled; // volatile 読み取り。core1 から呼ばれる。
}

#else

void mirror_filter_switch_init(void) {}
void mirror_filter_switch_poll(void) {}
bool mirror_filter_switch_is_enabled(void) { return false; }

#endif