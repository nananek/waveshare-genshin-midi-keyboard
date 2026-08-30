#include "usb_power_control.h"

#include <stdio.h>

#include "hardware/sync.h"
#include "pico/stdlib.h"

#include "config.h"
#include "status_led.h"

static usb_power_fsm_t s_fsm;
static usb_power_output_t s_last;
static bool s_initialized;
static volatile bool s_fault_latched;

static void latch_fault(void) {
    // Safe from both the GPIO IRQ and core0 task context: EN is forced low
    // before deferred logging or LED work.
    gpio_put(PIN_USB_POWER_EN, false);
    s_fault_latched = true;
}

static void fault_gpio_irq(uint gpio, uint32_t events) {
    if (gpio == PIN_USB_POWER_FAULT_N &&
        (events & GPIO_IRQ_EDGE_FALL) != 0) {
        latch_fault();
    }
}

void usb_power_control_init(uint32_t now_ms) {
    // A normal software path must not be able to clear a latched fault by
    // re-running init. BSS is cleared only by a real MCU reset/reboot.
    if (s_initialized) {
        return;
    }
    s_initialized = true;

    gpio_init(PIN_USB_POWER_EN);
    gpio_put(PIN_USB_POWER_EN, false);
    gpio_set_dir(PIN_USB_POWER_EN, GPIO_OUT);

    gpio_init(PIN_USB_POWER_FAULT_N);
    gpio_set_dir(PIN_USB_POWER_FAULT_N, GPIO_IN);
    gpio_disable_pulls(PIN_USB_POWER_FAULT_N);

    usb_power_fsm_init(&s_fsm);
    s_fault_latched = false;
    gpio_set_irq_enabled_with_callback(PIN_USB_POWER_FAULT_N,
                                       GPIO_IRQ_EDGE_FALL, true,
                                       fault_gpio_irq);
    // A line already low when IRQ capture is enabled has no falling edge, so
    // sample it explicitly. This also covers FAULT asserted during boot.
    if (!gpio_get(PIN_USB_POWER_FAULT_N)) {
        latch_fault();
    }
    usb_power_output_t initial = usb_power_fsm_step(
        &s_fsm, false, !s_fault_latched);
    s_last = (usb_power_output_t){
        .state = initial.state,
        .enable = initial.enable,
    };
    status_led_init();
    status_led_set_usb_power_state(s_last.state, now_ms);
}

usb_power_output_t usb_power_control_task(uint32_t now_ms,
                                          bool upstream_ready) {
    bool fault_n = !s_fault_latched && gpio_get(PIN_USB_POWER_FAULT_N);
    usb_power_output_t next = usb_power_fsm_step(
        &s_fsm, upstream_ready, fault_n);

    if (next.state == USB_POWER_FAULT_LATCHED) {
        latch_fault();
    } else if (next.enable) {
        // Serialize the final check and EN assertion with the falling-edge
        // callback. Thus the task can never reassert EN after that callback
        // has completed; an edge during this short section runs immediately
        // when interrupts are restored and forces EN low.
        bool enabled = false;
        uint32_t irq_state = save_and_disable_interrupts();
        if (!s_fault_latched && gpio_get(PIN_USB_POWER_FAULT_N)) {
            gpio_put(PIN_USB_POWER_EN, true);
            enabled = true;
        }
        restore_interrupts(irq_state);
        if (!enabled || s_fault_latched ||
            !gpio_get(PIN_USB_POWER_FAULT_N)) {
            latch_fault();
            next = usb_power_fsm_step(&s_fsm, upstream_ready, false);
        }
    } else {
        gpio_put(PIN_USB_POWER_EN, false);
    }

    if (next.state != s_last.state) {
        printf("[usb-power] %s EN=%u FAULT_N=%u\r\n",
               usb_power_state_name(next.state), next.enable ? 1u : 0u,
               fault_n ? 1u : 0u);
        status_led_set_usb_power_state(next.state, now_ms);
    }
    status_led_task(now_ms);
    s_last = next;
    return next;
}
