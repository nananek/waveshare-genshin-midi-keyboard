#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hid_mute.h"
#include "config.h"

// ===========================================================================
//  ミュートスイッチ入力 (ハード依存層)。
//  内部プルアップを常時有効化し、GP28 ↔ GND のトグルスイッチを既定配線とする。
//  閉じる (LOW) = アクティブ = ミュート。デバウンスは時間ベース (連続観測)。
//  無効時 (MUTE_SWITCH_ENABLE=0) は no-op になり、コードもコンパイルアウトされる。
// ===========================================================================

#if MUTE_SWITCH_ENABLE

// GPIO 読み取りを「アクティブ(= ミュート)なら true」に正規化する
static bool read_active(void) {
    return gpio_get(MUTE_SWITCH_PIN) == (bool)MUTE_SWITCH_ACTIVE_LEVEL;
}

static bool s_committed;         // 確定済みミュート状態
static bool s_candidate;         // デバウンス対象の候補レベル (アクティブかどうか)
static bool s_sampled;           // 前回の生読み取り (変化の検知用)
static absolute_time_t s_candidate_since;

void hid_mute_init(void) {
    gpio_init(MUTE_SWITCH_PIN);
    gpio_set_dir(MUTE_SWITCH_PIN, GPIO_IN);
    gpio_pull_up(MUTE_SWITCH_PIN); // 開 = HIGH = 非ミュートが既定
    bool raw = read_active();
    s_committed = raw;             // 起動直後は現在値を確定扱い (エッジは発生しない)
    s_candidate = raw;
    s_sampled = raw;
    s_candidate_since = get_absolute_time();
}

hid_mute_edge_t hid_mute_poll(void) {
    bool raw = read_active();

    // 生レベルが揺れている間は何もしない (チャタリング)
    if (raw != s_sampled) {
        s_sampled = raw;
        return HID_MUTE_NO_CHANGE;
    }

    // 直前の生読み取りと同じなら候補を更新して計時開始
    if (raw != s_candidate) {
        s_candidate = raw;
        s_candidate_since = get_absolute_time();
        return HID_MUTE_NO_CHANGE;
    }

    // MUTE_SWITCH_DEBOUNCE_MS 連続で同じレベル → 確定 (エッジ)
    if (s_candidate != s_committed &&
        to_ms_since_boot(get_absolute_time()) - to_ms_since_boot(s_candidate_since)
            >= (uint32_t)MUTE_SWITCH_DEBOUNCE_MS) {
        s_committed = s_candidate;
        return s_committed ? HID_MUTE_ENTER : HID_MUTE_EXIT;
    }

    return HID_MUTE_NO_CHANGE;
}

bool hid_mute_is_muted(void) {
    return s_committed;
}

#else // 無効時は常に非ミュート (コードは残すが何もしない)

void hid_mute_init(void) {}
hid_mute_edge_t hid_mute_poll(void) { return HID_MUTE_NO_CHANGE; }
bool hid_mute_is_muted(void) { return false; }

#endif