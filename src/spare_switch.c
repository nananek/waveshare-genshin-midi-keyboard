#include "spare_switch.h"

#include "debounced_switch.h"
#include "config.h"

#if SPARE_SWITCH_ENABLE

static debounced_switch_t s_sw;
static bool s_active;

void spare_switch_init(void) {
    debounced_switch_init(&s_sw, SPARE_SWITCH_PIN,
                          (bool)SPARE_SWITCH_ACTIVE_LEVEL,
                          (uint32_t)SPARE_SWITCH_DEBOUNCE_MS);
    s_active = debounced_switch_is_active(&s_sw);
}

spare_switch_edge_t spare_switch_poll(void) {
    debounced_switch_edge_t edge = debounced_switch_poll(&s_sw);
    s_active = debounced_switch_is_active(&s_sw);
    switch (edge) {
    case DEBOUNCED_SWITCH_ENTER: return SPARE_SWITCH_ENTER;
    case DEBOUNCED_SWITCH_EXIT:  return SPARE_SWITCH_EXIT;
    default:                     return SPARE_SWITCH_NO_CHANGE;
    }
}

bool spare_switch_is_active(void) {
    return s_active;
}

#else

void spare_switch_init(void) {}
spare_switch_edge_t spare_switch_poll(void) { return SPARE_SWITCH_NO_CHANGE; }
bool spare_switch_is_active(void) { return false; }

#endif
