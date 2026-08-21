#include "sustain_switch.h"
#include "../src/debounced_switch.h"
#include "config.h"

// ===========================================================================
//  サステインスイッチ入力 (ハード依存層、issue #8)。デバウンス本体は
//  src/debounced_switch.c を共用する (reset_button.c と同じ理由、CLAUDE.md参照)。
//  無効時 (SUSTAIN_SWITCH_ENABLE=0) は no-op になりコンパイルアウトされる。
// ===========================================================================

#if SUSTAIN_SWITCH_ENABLE

static debounced_switch_t s_sw;

void sustain_switch_init(void) {
    debounced_switch_init(&s_sw, SUSTAIN_SWITCH_PIN, (bool)SUSTAIN_SWITCH_ACTIVE_LEVEL,
                           (uint32_t)SUSTAIN_SWITCH_DEBOUNCE_MS);
}

sustain_switch_edge_t sustain_switch_poll(void) {
    switch (debounced_switch_poll(&s_sw)) {
    case DEBOUNCED_SWITCH_ENTER: return SUSTAIN_SWITCH_ENTER;
    case DEBOUNCED_SWITCH_EXIT:  return SUSTAIN_SWITCH_EXIT;
    default:                     return SUSTAIN_SWITCH_NO_CHANGE;
    }
}

#else

void sustain_switch_init(void) {}
sustain_switch_edge_t sustain_switch_poll(void) { return SUSTAIN_SWITCH_NO_CHANGE; }

#endif
