#include "hid_mute.h"
#include "debounced_switch.h"
#include "config.h"

// ===========================================================================
//  ミュートスイッチ入力 (ハード依存層)。デバウンス本体は debounced_switch を使う。
//  既定配線: GP28 ↔ GND のトグルスイッチ。閉じる (LOW) = アクティブ = ミュート。
//  無効時 (MUTE_SWITCH_ENABLE=0) は no-op になり、コードもコンパイルアウトされる。
// ===========================================================================

#if MUTE_SWITCH_ENABLE

static debounced_switch_t s_sw;

void hid_mute_init(void) {
    debounced_switch_init(&s_sw, MUTE_SWITCH_PIN, (bool)MUTE_SWITCH_ACTIVE_LEVEL,
                          (uint32_t)MUTE_SWITCH_DEBOUNCE_MS);
}

hid_mute_edge_t hid_mute_poll(void) {
    switch (debounced_switch_poll(&s_sw)) {
    case DEBOUNCED_SWITCH_ENTER: return HID_MUTE_ENTER;
    case DEBOUNCED_SWITCH_EXIT:  return HID_MUTE_EXIT;
    default:                     return HID_MUTE_NO_CHANGE;
    }
}

bool hid_mute_is_muted(void) {
    return debounced_switch_is_active(&s_sw);
}

#else // 無効時は常に非ミュート (コードは残すが何もしない)

void hid_mute_init(void) {}
hid_mute_edge_t hid_mute_poll(void) { return HID_MUTE_NO_CHANGE; }
bool hid_mute_is_muted(void) { return false; }

#endif