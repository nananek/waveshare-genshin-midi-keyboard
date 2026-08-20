#include "mirror_filter_switch.h"
#include "debounced_switch.h"
#include "config.h"

// ===========================================================================
//  楽器モードスイッチ入力 (ハード依存層)。デバウンス本体は debounced_switch を使う。
//  デバウンス確定値を volatile 共有フラグに書き、core1 からも読む。
// ===========================================================================

#if LYRE_SWITCH_ENABLE

static debounced_switch_t s_sw;
static volatile bool s_enabled;

void mirror_filter_switch_init(void) {
    debounced_switch_init(&s_sw, LYRE_SWITCH_PIN,
                          (bool)LYRE_SWITCH_ACTIVE_LEVEL,
                          (uint32_t)LYRE_SWITCH_DEBOUNCE_MS);
    s_enabled = debounced_switch_is_active(&s_sw);
}

void mirror_filter_switch_poll(void) {
    if (debounced_switch_poll(&s_sw) != DEBOUNCED_SWITCH_NO_CHANGE) {
        s_enabled = debounced_switch_is_active(&s_sw);
    }
}

bool mirror_filter_switch_is_enabled(void) {
    return s_enabled;
}

#else

void mirror_filter_switch_init(void) {}
void mirror_filter_switch_poll(void) {}
bool mirror_filter_switch_is_enabled(void) { return false; }

#endif
