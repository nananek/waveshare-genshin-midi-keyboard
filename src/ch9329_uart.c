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
static uint32_t s_baud;

static void flush_rx(void) {
    while (uart_is_readable(CH9329_UART)) {
        (void)uart_getc(CH9329_UART);
    }
}

// GET_INFO を送り、ヘッダとチェックサムが正しい応答が返るか調べる。
// CH9329 は SET を GND に落とさなくても GET_INFO に応答する。
static bool probe_baud(uint32_t baud) {
    uart_set_baudrate(CH9329_UART, baud);
    flush_rx();

    uint8_t frame[CH9329_FRAME_MAX];
    uint8_t len = ch9329_build_get_info_frame(CH9329_ADDR_DEFAULT, frame);
    uart_write_blocking(CH9329_UART, frame, len);

    uint8_t rx[32];
    uint32_t count = 0;
    absolute_time_t deadline = make_timeout_time_ms(CH9329_BAUD_PROBE_MS);
    while (!time_reached(deadline)) {
        while (uart_is_readable(CH9329_UART) && count < sizeof(rx)) {
            rx[count++] = (uint8_t)uart_getc(CH9329_UART);
        }
        // 先頭のゴミを読み飛ばしてヘッダを探す。
        uint32_t start = 0;
        while (start + 1u < count &&
               !(rx[start] == CH9329_HEADER_0 && rx[start + 1u] == CH9329_HEADER_1)) {
            start++;
        }
        if (count - start >= 6u && ch9329_frame_valid(&rx[start], count - start)) {
            return true;
        }
        sleep_us(100);
    }
    return false;
}

void ch9329_uart_init(void) {
    s_baud = CH9329_UART_BAUD;
    uart_init(CH9329_UART, s_baud);
    gpio_set_function(CH9329_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(CH9329_UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(CH9329_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(CH9329_UART, true);

#if CH9329_AUTO_BAUD
    static const uint32_t candidates[] = CH9329_BAUD_CANDIDATES;
    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        if (probe_baud(candidates[i])) {
            s_baud = candidates[i];
            break;
        }
    }
    uart_set_baudrate(CH9329_UART, s_baud);
    flush_rx();
#endif

    s_has_next = false;
    s_sending = false;
    s_tx_len = 0;
    s_tx_pos = 0;
}

uint32_t ch9329_uart_baud(void) {
    return s_baud;
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
