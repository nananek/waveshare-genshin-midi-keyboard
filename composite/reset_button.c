#include "reset_button.h"
#include "../src/debounced_switch.h"
#include "config.h"

#if COMPOSITE_RESET_ENABLE

static debounced_switch_t s_sw;

void reset_button_init(void) {
    debounced_switch_init(&s_sw, COMPOSITE_RESET_PIN, (bool)COMPOSITE_RESET_ACTIVE_LEVEL,
                           (uint32_t)COMPOSITE_RESET_DEBOUNCE_MS);
}

bool reset_button_poll(void) {
    return debounced_switch_poll(&s_sw) == DEBOUNCED_SWITCH_ENTER;
}

#else

void reset_button_init(void) {}
bool reset_button_poll(void) { return false; }

#endif
