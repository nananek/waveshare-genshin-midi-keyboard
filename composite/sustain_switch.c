#include "sustain_switch.h"
#include "../src/debounced_switch.h"
#include "config.h"

#if COMPOSITE_SUSTAIN_ENABLE

static debounced_switch_t s_sw;

void sustain_switch_init(void) {
    debounced_switch_init(&s_sw, COMPOSITE_SUSTAIN_PIN, (bool)COMPOSITE_SUSTAIN_ACTIVE_LEVEL,
                           (uint32_t)COMPOSITE_SUSTAIN_DEBOUNCE_MS);
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
