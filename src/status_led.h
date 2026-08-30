#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <stdint.h>

#include "usb_power_fsm.h"

// Waveshare RP2350-Zero onboard WS2812B (DIN=GP16).  Colours are deliberately
// dim so the indicator does not materially consume the upstream USB budget.
void status_led_init(void);
void status_led_set_usb_power_state(usb_power_state_t state, uint32_t now_ms);
void status_led_task(uint32_t now_ms);

#endif // STATUS_LED_H
