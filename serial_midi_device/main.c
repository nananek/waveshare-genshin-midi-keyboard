#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "tusb.h"
#include "config.h"

// ===========================================================================
//  シリアル→USB-MIDI ブリッジ (ボード2)。
//  UART RX (UART1 / GP5 / 31250 8N1) で受けた MIDI バイト列をそのまま
//  native USB = USB-MIDI クラスデバイスとして PC/DAW へ送る。
//  シングルコア (core0)。PIO-USB ホストが無いので set_sys_clock_khz は不要。
//  デバッグログは UART0 (GP0/GP1 @115200)、MIDI RX は UART1 と分離。
//
//  受信バイト列はボード1 (ミラー) がそのまま送る生ストリームなので、
//  ここで再パース・再ステータス付与はしない。
// ===========================================================================

#if MIDI_UART_INDEX == 0
#define MIDI_UART uart0
#else
#define MIDI_UART uart1
#endif

// 未送信分の持ち越しバッファ (tud_midi の TX FIFO が一杯のとき残りを保持)。
// USB 未接続 / FIFO 満杯時のデータは落とす (ミラー用途の既知の制限)。
static uint8_t s_carry[64];
static uint32_t s_carry_len = 0;

static void flush_carry(void) {
    while (s_carry_len > 0) {
        uint32_t n = tud_midi_stream_write(0, s_carry, s_carry_len);
        if (n == 0) {
            return; // FIFO 満杯 → 次回リトライ
        }
        memmove(s_carry, s_carry + n, s_carry_len - n);
        s_carry_len -= n;
    }
}

int main(void) {
    stdio_init_all(); // デバッグログ: UART0 (GP0/GP1) @115200
    printf("\r\n[boot] Serial MIDI Bridge: RX=GP%u @%u\r\n",
           MIDI_UART_RX_PIN, MIDI_UART_BAUD);

    // MIDI RX: UART (GP5) @31250 8N1。デバッグ UART0 と分離。
    uart_init(MIDI_UART, MIDI_UART_BAUD);
    gpio_set_function(MIDI_UART_RX_PIN, GPIO_FUNC_UART);

    tud_init(BOARD_TUD_RHPORT);

    for (;;) {
        tud_task();
        flush_carry();

        // UART RX FIFO をドレインして USB-MIDI へ流す
        uint8_t buf[64];
        uint32_t i = 0;
        while (i < sizeof(buf) && uart_is_readable(MIDI_UART)) {
            buf[i++] = (uint8_t)uart_getc(MIDI_UART);
        }
        if (i > 0) {
            uint32_t n = tud_midi_stream_write(0, buf, i);
            if (n < i && s_carry_len == 0 && (i - n) <= sizeof(s_carry)) {
                memcpy(s_carry, buf + n, i - n);
                s_carry_len = i - n;
            }
            // 持ち越し中にもう一回来た場合は古い方を捨てる (ミラー用途では許容)
        }
    }
}