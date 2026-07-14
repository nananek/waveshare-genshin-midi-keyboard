#include <stdio.h>
#include "tusb.h"
#include "midi_host.h"
#include "midi_parse.h"
#include "config.h"

#ifdef PICO_DEFAULT_LED_PIN
#include "pico/stdlib.h"
#endif

// core1 専用。tuh_task のコールバック文脈でのみ触るので排他不要。
static midi_parser_t s_parser;

static void led_set(bool on) {
#ifdef PICO_DEFAULT_LED_PIN
    gpio_put(PICO_DEFAULT_LED_PIN, on ? 1 : 0);
#else
    (void)on;
#endif
}

void midi_host_init(void) {
    midi_parser_init(&s_parser);
#ifdef PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif
    led_set(false);
}

// midi_parse からの Note コールバック。チャンネルフィルタを適用してアプリへ。
static void on_note(uint8_t channel, uint8_t note, bool on, void *ctx) {
    (void)ctx;
#if MIDI_CHANNEL_FILTER != 0
    if (channel != (uint8_t)(MIDI_CHANNEL_FILTER - 1)) {
        return;
    }
#else
    (void)channel;
#endif
    app_on_midi_note(note, on);
}

// ===========================================================================
//  TinyUSB MIDI ホストコールバック (idx ベース、hathach/tinyusb 0.20.0+)
//  ※ 旧 API は dev_addr 先頭だった。upstream 0.20.0 以降は idx 先頭 +
//    mount は tuh_midi_mount_cb_t 構造体を受け取る。
// ===========================================================================

void tuh_midi_mount_cb(uint8_t idx, const tuh_midi_mount_cb_t *mount_cb_data) {
    printf("[midi] mounted idx=%u daddr=%u rx_cables=%u tx_cables=%u\r\n",
           idx, mount_cb_data->daddr,
           mount_cb_data->rx_cable_count, mount_cb_data->tx_cable_count);
    midi_parser_init(&s_parser); // 新デバイス: ランニングステータス等をリセット
    led_set(true);
}

void tuh_midi_umount_cb(uint8_t idx) {
    printf("[midi] unmounted idx=%u\r\n", idx);
    app_on_midi_disconnect(); // 全キー解放 (ホットプラグ)
    led_set(false);
}

void tuh_midi_rx_cb(uint8_t idx, uint32_t xferred_bytes) {
    if (xferred_bytes == 0) {
        return;
    }
    // stream_read は USB-MIDI パケットを 1.0 バイトストリームへ復元して返す。
    uint8_t buffer[64];
    uint8_t cable = 0;
    for (;;) {
        uint32_t n = tuh_midi_stream_read(idx, &cable, buffer, (uint16_t)sizeof(buffer));
        if (n == 0) {
            break;
        }
        midi_parser_push(&s_parser, buffer, n, on_note, NULL);
    }
}
