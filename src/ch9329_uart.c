#include "ch9329_uart.h"

#include <string.h>

#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"

#include "config.h"

#if CH9329_UART_INDEX == 0
#define CH9329_UART uart0
#else
#define CH9329_UART uart1
#endif

static uint8_t s_next[CH9329_FRAME_MAX];
static bool s_has_next;
static uint8_t s_tx[CH9329_FRAME_MAX];
static uint8_t s_tx_len;
static uint8_t s_tx_pos;
static bool s_sending;

void ch9329_uart_init(void) {
    uart_init(CH9329_UART, CH9329_UART_BAUD);
    gpio_set_function(CH9329_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(CH9329_UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(CH9329_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(CH9329_UART, true);
    s_has_next = false;
    s_sending = false;
    s_tx_len = 0;
    s_tx_pos = 0;
}

void ch9329_uart_queue_report(uint8_t modifier,
                              const uint8_t keys[CH9329_KEY_COUNT]) {
    // 送信中のフレームは壊さず、その後ろに「最新の状態」だけを残す。
    s_has_next = true;
    ch9329_build_keyboard_frame(modifier, keys, s_next);
}

void ch9329_uart_task(void) {
    if (!s_sending) {
        if (!s_has_next) {
            return;
        }
        memcpy(s_tx, s_next, CH9329_KEYBOARD_FRAME_LEN);
        s_tx_len = CH9329_KEYBOARD_FRAME_LEN;
        s_tx_pos = 0;
        s_has_next = false;
        s_sending = true;
    }

    while (s_tx_pos < s_tx_len && uart_is_writable(CH9329_UART)) {
        uart_putc_raw(CH9329_UART, (char)s_tx[s_tx_pos]);
        s_tx_pos++;
    }
    if (s_tx_pos >= s_tx_len) {
        s_sending = false;
    }
}

bool ch9329_uart_busy(void) {
    return s_sending || s_has_next;
}
