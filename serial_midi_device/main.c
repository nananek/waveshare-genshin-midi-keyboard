#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "tusb.h"
#include "config.h"
#include "reset_button.h"
#include "sustain_switch.h"

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

#define MIDI_CABLE_NUM 0 // 既存の tud_midi_stream_write(0, ...) と揃える

// issue #6/#8: tud_midi_stream_write() の内部状態機械 (stream->index/total,
// lib/tinyusb/src/class/midi/midi_device.c:189-283) を経由すると、UARTパス
// スルー中の未完了メッセージ (続きバイト待ち) に割り込んで壊す恐れがある
// (on-going packet中は届いたバイトを無条件でバッファへ追記するため、割り込ん
// だバイトが「次のデータバイト」と誤解釈される)。tud_midi_packet_write() は
// この状態機械を経由せず生の4バイトUSB-MIDIイベントを直接送るため、パス
// スルーの途中状態に一切影響しない。CIN値は tud_midi_stream_write 自身が
// 同じ種類のメッセージに使う値と揃えてある (同 midi_device.c:219-239 参照)。
static void send_realtime_packet(uint8_t status_byte) {
    uint8_t packet[4] = {
        (uint8_t)((MIDI_CABLE_NUM << 4) | MIDI_CIN_SYSEX_END_1BYTE), // =5, 1byte system msg
        status_byte, 0, 0,
    };
    tud_midi_packet_write(packet);
}

static void send_cc_packet(uint8_t channel, uint8_t cc_num, uint8_t value) {
    uint8_t packet[4] = {
        (uint8_t)((MIDI_CABLE_NUM << 4) | MIDI_CIN_CONTROL_CHANGE),
        (uint8_t)(0xB0 | (channel & 0x0F)), // Control Change, channel 0-15
        cc_num,
        value,
    };
    tud_midi_packet_write(packet);
}

int main(void) {
    stdio_init_all(); // デバッグログ: UART0 (GP0/GP1) @115200
    printf("\r\n[boot] Serial MIDI Bridge: RX=GP%d @%d\r\n",
           MIDI_UART_RX_PIN, MIDI_UART_BAUD);

    // MIDI RX: UART (GP5) @31250 8N1。デバッグ UART0 と分離。
    uart_init(MIDI_UART, MIDI_UART_BAUD);
    gpio_set_function(MIDI_UART_RX_PIN, GPIO_FUNC_UART);

    tud_init(BOARD_TUD_RHPORT);
    reset_button_init();
    sustain_switch_init();

    for (;;) {
        tud_task();
        flush_carry();

        if (reset_button_poll()) {
            send_realtime_packet(MIDI_STATUS_SYSREAL_SYSTEM_RESET);
            printf("[reset] MIDI System Reset sent\r\n");
        }

        switch (sustain_switch_poll()) {
        case SUSTAIN_SWITCH_ENTER:
            send_cc_packet(SUSTAIN_MIDI_CHANNEL, 64, 127);
            printf("[sustain] ON (CC64=127)\r\n");
            break;
        case SUSTAIN_SWITCH_EXIT:
            send_cc_packet(SUSTAIN_MIDI_CHANNEL, 64, 0);
            printf("[sustain] OFF (CC64=0)\r\n");
            break;
        default:
            break;
        }

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
            // 持ち越しが残っている間に新データが来た場合は新しい方を捨てる
            // (ミラー用途では許容。古い方を残すのは tud_midi のストリーム状態を
            //  正しく継続させるため)
        }
    }
}
