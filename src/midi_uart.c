#include "midi_uart.h"

#include "hardware/gpio.h"
#include "pico/sync.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"

#include "config.h"

#if MIDI_UART_INDEX == 0
#define MIDI_UART uart0
#else
#define MIDI_UART uart1
#endif

static mutex_t s_uart_mutex;
static bool s_ready;

void midi_uart_init(void) {
    mutex_init(&s_uart_mutex);
    uart_init(MIDI_UART, MIDI_UART_BAUD);
    gpio_set_function(MIDI_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(MIDI_UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(MIDI_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(MIDI_UART, true);
    s_ready = true;
}

void midi_uart_send(const uint8_t *bytes, uint32_t length) {
    if (!s_ready || bytes == NULL || length == 0) {
        return;
    }
    // 31250 bps では 1 バイト約 320 us。大きな SysEx 中は core1 の
    // tuh_task が数十 ms 止まり得るが、CH345 への RAW パススルーなので
    // パースや変換は不要。既知のトレードオフとして許容する。
    // core0 からの CC 送信とバイト列が混ざらないよう mutex で直列化する。
    mutex_enter_blocking(&s_uart_mutex);
    uart_write_blocking(MIDI_UART, bytes, length);
    mutex_exit(&s_uart_mutex);
}

void midi_uart_send_cc(uint8_t channel, uint8_t cc, uint8_t value) {
    uint8_t msg[3] = {
        (uint8_t)(0xB0u | (channel & 0x0Fu)),
        (uint8_t)(cc & 0x7Fu),
        (uint8_t)(value & 0x7Fu),
    };
    midi_uart_send(msg, sizeof(msg));
}
