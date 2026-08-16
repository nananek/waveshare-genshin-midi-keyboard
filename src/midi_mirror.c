#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "midi_mirror.h"
#include "config.h"

// ===========================================================================
//  RAW MIDI の UART ミラー出力 (ハード依存層)。
//  tuh_midi_stream_read が返す生 MIDI バイト列をそのまま別 UART へ流す。
//
//  ※ ブロッキングのトレードオフ: uart_write_blocking は TX FIFO に詰めるまで
//    コア1 (tuh_task) を止める。31250 baud では 1 バイト ≈ 320µs。
//    - Note On/Off (3 バイト) や CC クラス: 1ms 未満。演奏用途で問題なし。
//    - 大きな SysEx ダンプ: 64 バイトで ~20ms ほど止まるが、ミラー用途として許容。
// ===========================================================================

#if MIDI_UART_MIRROR_ENABLE

// UART インスタンス選択 (コンパイル時分岐で uart0/uart1 を解決)
#if MIDI_UART_MIRROR_UART == 0
#define MIRROR_UART uart0
#else
#define MIRROR_UART uart1
#endif

void midi_mirror_init(void) {
    uart_init(MIRROR_UART, MIDI_UART_MIRROR_BAUD); // 8N1 がデフォルト
    gpio_set_function(MIDI_UART_MIRROR_TX_PIN, GPIO_FUNC_UART);
    uart_set_fifo_enabled(MIRROR_UART, true);
}

void midi_mirror_send(const uint8_t *data, uint32_t len) {
    if (len == 0) {
        return;
    }
    uart_write_blocking(MIRROR_UART, data, len);
}

#else // 無効時は no-op (コードは残すが何もしない)

void midi_mirror_init(void) {}
void midi_mirror_send(const uint8_t *data, uint32_t len) { (void)data; (void)len; }

#endif
