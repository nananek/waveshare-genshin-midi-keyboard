#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "mirror_filter_switch.h"
#include "config.h"

// ===========================================================================
//  ミラーフィルタースイッチ入力 (ハード依存層)。
//  内部プルアップを常時有効化し、GP29 ↔ GND のトグルスイッチを既定配線とする。
//  閉じる (LOW) = アクティブ = フィルター ON。デバウンスは時間ベース (連続観測)。
//  デバウンス確定値を volatile 共有フラグに書き、core1 (midi_mirror) が読む。
//  無効時 (MIRROR_FILTER_SWITCH_ENABLE=0) は no-op + 常にパススルー。
// ===========================================================================

#if MIRROR_FILTER_SWITCH_ENABLE

static volatile bool s_enabled;              // core0 が書き、core1 が読む共有フラグ
static bool s_committed, s_candidate, s_sampled;
static absolute_time_t s_candidate_since;

static bool read_active(void) {
    return gpio_get(MIRROR_FILTER_SWITCH_PIN) == (bool)MIRROR_FILTER_SWITCH_ACTIVE_LEVEL;
}

void mirror_filter_switch_init(void) {
    gpio_init(MIRROR_FILTER_SWITCH_PIN);
    gpio_set_dir(MIRROR_FILTER_SWITCH_PIN, GPIO_IN);
    gpio_pull_up(MIRROR_FILTER_SWITCH_PIN);
    bool raw = read_active();
    s_committed = raw;
    s_candidate = raw;
    s_sampled = raw;
    s_candidate_since = get_absolute_time();
    s_enabled = s_committed;
}

void mirror_filter_switch_poll(void) {
    bool raw = read_active();
    if (raw != s_sampled) {
        s_sampled = raw;
        return;
    }
    if (raw != s_candidate) {
        s_candidate = raw;
        s_candidate_since = get_absolute_time();
        return;
    }
    if (s_candidate != s_committed &&
        to_ms_since_boot(get_absolute_time()) - to_ms_since_boot(s_candidate_since)
            >= (uint32_t)MIRROR_FILTER_SWITCH_DEBOUNCE_MS) {
        s_committed = s_candidate;
        s_enabled = s_committed; // デバウンス確定時に共有フラグへ反映
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