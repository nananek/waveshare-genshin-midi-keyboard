#include "status_led.h"

#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"

#include "config.h"
#include "ws2812.pio.h"

static PIO const s_pio = pio2;
static uint s_sm;
static usb_power_state_t s_state;
static uint32_t s_state_since_ms;
static bool s_pixel_on;

static uint32_t grb(uint8_t red, uint8_t green, uint8_t blue) {
    return ((uint32_t)green << 24) |
           ((uint32_t)red << 16) |
           ((uint32_t)blue << 8);
}

static void set_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    // The PIO program stalls with the data line low after 24 bits, providing
    // the WS2812 latch interval without sleeping or blocking TinyUSB work.
    if (!pio_sm_is_tx_fifo_full(s_pio, s_sm)) {
        pio_sm_put(s_pio, s_sm, grb(red, green, blue));
    }
}

void status_led_init(void) {
    uint offset = pio_add_program(s_pio, &ws2812_program);
    s_sm = pio_claim_unused_sm(s_pio, true);
    ws2812_program_init(s_pio, s_sm, offset, PIN_STATUS_WS2812, 800000.0f);
    s_state = USB_POWER_WAIT_UPSTREAM;
    s_state_since_ms = 0;
    s_pixel_on = false;
    set_rgb(0, 0, 0);
}

void status_led_set_usb_power_state(usb_power_state_t state, uint32_t now_ms) {
    s_state = state;
    s_state_since_ms = now_ms;
    s_pixel_on = false;

    switch (state) {
    case USB_POWER_WAIT_UPSTREAM:
        set_rgb(0, 0, 0);       // pre-enumeration/suspend: LEDも消灯
        break;
    case USB_POWER_ON:
        set_rgb(0, 4, 0);       // downstream VBUS enabled
        s_pixel_on = true;
        break;
    case USB_POWER_FAULT_LATCHED:
        set_rgb(8, 0, 0);       // start 1 Hz, 25% duty fault blink
        s_pixel_on = true;
        break;
    }
}

void status_led_task(uint32_t now_ms) {
    if (s_state != USB_POWER_FAULT_LATCHED) {
        return;
    }

    bool should_be_on = (uint32_t)(now_ms - s_state_since_ms) % 1000u < 250u;
    if (should_be_on != s_pixel_on) {
        set_rgb(should_be_on ? 8u : 0u, 0, 0);
        s_pixel_on = should_be_on;
    }
}
