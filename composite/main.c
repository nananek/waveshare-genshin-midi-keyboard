#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/clocks.h"

#include "tusb.h"
#include "pio_usb.h"

#include "config.h"
#include "hid_device.h"
#include "hid_mute.h"
#include "midi_host.h"
#include "mirror_filter_switch.h"
#include "midi_bridge.h"
#include "reset_button.h"
#include "sustain_switch.h"

// ===========================================================================
//  composite 1枚化: MIDI(host, core1) → HID(device, core0) + USB-MIDI(device)
//  イベントはロックフリーキューでコア間受け渡し。HID 状態は core0 のみが触る。
//  src/main.c のフォーク。差分:
//    - midi_bridge_task で RAW MIDI を USB-MIDI へ転送
//    - RESET/SUSTAIN ボタンを GP26/27 で統合 (packet_write)
// ===========================================================================

#define EVT_RELEASE_ALL 0xFFu

typedef struct {
    uint8_t note;
    uint8_t on;
} midi_evt_t;

static queue_t s_evt_queue;
static bool s_note_held[128];

void app_on_midi_note(uint8_t note, bool on) {
    midi_evt_t e = { .note = note, .on = on ? 1u : 0u };
    queue_try_add(&s_evt_queue, &e);
}

void app_on_midi_disconnect(void) {
    midi_evt_t e = { .note = EVT_RELEASE_ALL, .on = 0 };
    queue_try_add(&s_evt_queue, &e);
}

// ---------------------------------------------------------------------------
//  core1: PIO-USB ホスト (rhport 1)
// ---------------------------------------------------------------------------
static void core1_main(void) {
    sleep_ms(10);

    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = PIN_USB_HOST_DP;
    tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
    tuh_init(BOARD_TUH_RHPORT);

    midi_host_init();

    for (;;) {
        tuh_task();
    }
}

// ---------------------------------------------------------------------------
//  core0: ネイティブ USB デバイス (rhport 0) = HID + MIDI composite
// ---------------------------------------------------------------------------
int main(void) {
    set_sys_clock_khz(120000, true);

    stdio_init_all();
    printf("\r\n[boot] Genshin MIDI Composite: D+=GP%d\r\n", PIN_USB_HOST_DP);

    queue_init(&s_evt_queue, sizeof(midi_evt_t), 128);

    tud_init(BOARD_TUD_RHPORT);
    hid_device_init();
    hid_mute_init();
    mirror_filter_switch_init();
    hid_device_set_sumeru_mode(mirror_filter_switch_is_enabled());
    midi_bridge_init();
    reset_button_init();
    sustain_switch_init();

    multicore_launch_core1(core1_main);

    for (;;) {
        tud_task();

        // 1) MIDI bridge の carry flush + queue drain → USB-MIDI 送信
        midi_bridge_task();

        // 2) RESET / SUSTAIN ボタン (core0 で packet_write)
        if (reset_button_poll()) {
            midi_bridge_send_realtime(0xFF);
            printf("[reset] MIDI System Reset sent\r\n");
        }

        switch (sustain_switch_poll()) {
        case SUSTAIN_SWITCH_ENTER:
            midi_bridge_send_cc(COMPOSITE_SUSTAIN_MIDI_CHANNEL, 64, 127);
            printf("[sustain] ON (CC64=127)\r\n");
            break;
        case SUSTAIN_SWITCH_EXIT:
            midi_bridge_send_cc(COMPOSITE_SUSTAIN_MIDI_CHANNEL, 64, 0);
            printf("[sustain] OFF (CC64=0)\r\n");
            break;
        default:
            break;
        }

        // --- ミュートスイッチ: デバウンス済みエッジを検出 ---
        hid_mute_edge_t edge = hid_mute_poll();
        if (edge == HID_MUTE_ENTER) {
            printf("[mute] ON\r\n");
            hid_device_release_all();
        } else if (edge == HID_MUTE_EXIT) {
            printf("[mute] OFF\r\n");
            for (int i = 0; i < 128; i++) {
                if (s_note_held[i]) {
                    hid_device_note_on((uint8_t)i);
                }
            }
        }

        // --- 楽器モードスイッチ: デバウンス + 共有フラグ反映 ---
        bool old_sumeru_mode = mirror_filter_switch_is_enabled();
        mirror_filter_switch_poll();
        bool new_sumeru_mode = mirror_filter_switch_is_enabled();
        if (new_sumeru_mode != old_sumeru_mode) {
            hid_device_release_all();
            hid_device_set_sumeru_mode(new_sumeru_mode);
            for (int i = 0; i < 128; i++) {
                if (s_note_held[i]) {
                    hid_device_note_on((uint8_t)i);
                }
            }
        }

        // core1 から届いた MIDI イベントを反映。
        bool muted = hid_mute_is_muted();
        midi_evt_t e;
        while (queue_try_remove(&s_evt_queue, &e)) {
            if (e.note == EVT_RELEASE_ALL) {
                for (int i = 0; i < 128; i++) {
                    s_note_held[i] = false;
                }
                if (!muted) {
                    hid_device_release_all();
                }
                continue;
            }
            s_note_held[e.note] = e.on;
            if (muted) {
                continue;
            }
            if (e.on) {
                hid_device_note_on(e.note);
            } else {
                hid_device_note_off(e.note);
            }
        }

        hid_device_task();
    }
}
