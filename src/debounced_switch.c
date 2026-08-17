#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "debounced_switch.h"

static bool read_active(const debounced_switch_t *s) {
    return gpio_get(s->pin) == (bool)s->active_level;
}

void debounced_switch_init(debounced_switch_t *s, uint8_t pin, bool active_level,
                            uint32_t debounce_ms) {
    s->pin = pin;
    s->active_level = active_level;
    s->debounce_ms = debounce_ms;

    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    if (active_level) {
        gpio_pull_down(pin); // アクティブ HIGH: 開放時は LOW (非アクティブ) を既定にする
    } else {
        gpio_pull_up(pin);   // アクティブ LOW: 開放時は HIGH (非アクティブ) を既定にする
    }

    bool raw = read_active(s);
    s->committed = raw; // 起動直後は現在値を確定扱い (エッジは発生しない)
    s->candidate = raw;
    s->sampled = raw;
    s->candidate_since = get_absolute_time();
}

debounced_switch_edge_t debounced_switch_poll(debounced_switch_t *s) {
    bool raw = read_active(s);

    // 生レベルが揺れている間は何もしない (チャタリング)
    if (raw != s->sampled) {
        s->sampled = raw;
        return DEBOUNCED_SWITCH_NO_CHANGE;
    }

    // 直前の生読み取りと同じなら候補を更新して計時開始
    if (raw != s->candidate) {
        s->candidate = raw;
        s->candidate_since = get_absolute_time();
        return DEBOUNCED_SWITCH_NO_CHANGE;
    }

    // debounce_ms 連続で同じレベル → 確定 (エッジ)
    if (s->candidate != s->committed &&
        to_ms_since_boot(get_absolute_time()) - to_ms_since_boot(s->candidate_since)
            >= s->debounce_ms) {
        s->committed = s->candidate;
        return s->committed ? DEBOUNCED_SWITCH_ENTER : DEBOUNCED_SWITCH_EXIT;
    }

    return DEBOUNCED_SWITCH_NO_CHANGE;
}

bool debounced_switch_is_active(const debounced_switch_t *s) {
    return s->committed;
}
