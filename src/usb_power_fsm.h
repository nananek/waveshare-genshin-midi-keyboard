#ifndef USB_POWER_FSM_H
#define USB_POWER_FSM_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    USB_POWER_WAIT_UPSTREAM = 0,
    USB_POWER_ON,
    USB_POWER_FAULT_LATCHED,
} usb_power_state_t;

typedef struct {
    usb_power_state_t state;
} usb_power_fsm_t;

typedef struct {
    usb_power_state_t state;
    bool enable;
} usb_power_output_t;

// Pure policy layer.  It intentionally has no Pico SDK dependency so every
// transition, including boot-default-off, can be exercised on the host.
void usb_power_fsm_init(usb_power_fsm_t *fsm);
usb_power_output_t usb_power_fsm_step(usb_power_fsm_t *fsm,
                                      bool upstream_ready,
                                      bool fault_n);
const char *usb_power_state_name(usb_power_state_t state);

#endif // USB_POWER_FSM_H
