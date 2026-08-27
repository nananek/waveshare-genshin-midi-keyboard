#ifndef SUSTAIN_SWITCH_H
#define SUSTAIN_SWITCH_H

typedef enum {
    SUSTAIN_SWITCH_NO_CHANGE = 0,
    SUSTAIN_SWITCH_ENTER,
    SUSTAIN_SWITCH_EXIT,
} sustain_switch_edge_t;

void sustain_switch_init(void);
sustain_switch_edge_t sustain_switch_poll(void);

#endif // SUSTAIN_SWITCH_H
