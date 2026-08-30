#ifndef USB_POWER_CONTROL_H
#define USB_POWER_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#include "usb_power_fsm.h"

// Hardware adapter for the pure state machine. init() always establishes a
// low output latch before changing GP9 to an output and is idempotent so it
// cannot clear a fault without a real MCU reset. The PCB pull-down is the
// independent reset-time guarantee.
void usb_power_control_init(uint32_t now_ms);
usb_power_output_t usb_power_control_task(uint32_t now_ms,
                                          bool upstream_ready);

#endif // USB_POWER_CONTROL_H
