#include "usb_power_fsm.h"

static bool state_enables_output(usb_power_state_t state) {
    return state == USB_POWER_ON;
}

void usb_power_fsm_init(usb_power_fsm_t *fsm) {
    fsm->state = USB_POWER_WAIT_UPSTREAM;
}

usb_power_output_t usb_power_fsm_step(usb_power_fsm_t *fsm,
                                      bool upstream_ready,
                                      bool fault_n) {
    // FAULT is the only transition with permanent priority. Once entered,
    // this state has no software exit; only a real RP2350 reboot calls init().
    if (fsm->state != USB_POWER_FAULT_LATCHED) {
        if (!fault_n) {
            fsm->state = USB_POWER_FAULT_LATCHED;
        } else if (upstream_ready) {
            fsm->state = USB_POWER_ON;
        } else {
            fsm->state = USB_POWER_WAIT_UPSTREAM;
        }
    }

    usb_power_output_t output = {
        .state = fsm->state,
        .enable = state_enables_output(fsm->state),
    };
    return output;
}

const char *usb_power_state_name(usb_power_state_t state) {
    switch (state) {
    case USB_POWER_WAIT_UPSTREAM: return "WAIT_UPSTREAM";
    case USB_POWER_ON: return "ON";
    case USB_POWER_FAULT_LATCHED: return "FAULT_LATCHED";
    default: return "UNKNOWN";
    }
}
