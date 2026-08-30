#include <stdbool.h>
#include <stdio.h>

#include "usb_power_fsm.h"

static int failures;

static void expect(const char *name, usb_power_output_t output,
                   usb_power_state_t state, bool enable) {
    if (output.state != state || output.enable != enable) {
        printf("FAIL %s: got state=%s en=%u\n", name,
               usb_power_state_name(output.state), output.enable);
        failures++;
    }
}

int main(void) {
    usb_power_fsm_t fsm;
    usb_power_fsm_init(&fsm);

    expect("boot-off", usb_power_fsm_step(&fsm, false, true),
           USB_POWER_WAIT_UPSTREAM, false);
    expect("configured-on", usb_power_fsm_step(&fsm, true, true),
           USB_POWER_ON, true);
    expect("suspend-off", usb_power_fsm_step(&fsm, false, true),
           USB_POWER_WAIT_UPSTREAM, false);
    expect("resume-on", usb_power_fsm_step(&fsm, true, true),
           USB_POWER_ON, true);

    // The first observed assertion latches immediately; there is no blanking
    // interval or retry state.
    expect("first-fault-latches", usb_power_fsm_step(&fsm, true, false),
           USB_POWER_FAULT_LATCHED, false);
    expect("fault-release-does-not-clear",
           usb_power_fsm_step(&fsm, true, true),
           USB_POWER_FAULT_LATCHED, false);
    expect("detach-does-not-clear", usb_power_fsm_step(&fsm, false, true),
           USB_POWER_FAULT_LATCHED, false);
    expect("resume-does-not-clear", usb_power_fsm_step(&fsm, true, true),
           USB_POWER_FAULT_LATCHED, false);
    expect("reconfigure-does-not-clear",
           usb_power_fsm_step(&fsm, false, true),
           USB_POWER_FAULT_LATCHED, false);
    expect("boot-button-does-not-clear",
           usb_power_fsm_step(&fsm, true, true),
           USB_POWER_FAULT_LATCHED, false);
    expect("sw3-does-not-clear",
           usb_power_fsm_step(&fsm, true, true),
           USB_POWER_FAULT_LATCHED, false);

    // Only construction after an actual MCU reboot clears RAM state.
    usb_power_fsm_init(&fsm);
    expect("reboot-returns-to-boot-off",
           usb_power_fsm_step(&fsm, false, true),
           USB_POWER_WAIT_UPSTREAM, false);
    expect("reboot-still-needs-configuration",
           usb_power_fsm_step(&fsm, true, true),
           USB_POWER_ON, true);

    // A fault already active during boot is sampled and latched before EN.
    usb_power_fsm_init(&fsm);
    expect("boot-active-fault", usb_power_fsm_step(&fsm, false, false),
           USB_POWER_FAULT_LATCHED, false);
    expect("boot-fault-release-stays-latched",
           usb_power_fsm_step(&fsm, true, true),
           USB_POWER_FAULT_LATCHED, false);

    if (failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    puts("ALL PASS");
    return 0;
}
